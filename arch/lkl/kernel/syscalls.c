#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/callbacks.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/net.h>

static long sys_call(long _f, long arg1, long arg2, long arg3, long arg4, long arg5);


/*
 * sys_mount (is sloppy?? and) copies a full page from dev_name, type and data
 * which can trigger page faults (which a normal kernel can safely
 * ignore). Make sure we don't trigger page fauls.
 */
asmlinkage long sys_safe_mount(char __user *dev_name, char __user *dir_name,
				char __user *type, unsigned long flags,
				void __user *data)
{
	int err;
	unsigned long _dev_name=0, _type=0, _data=0;

	err=-ENOMEM;
	if (dev_name) {
		_dev_name=__get_free_page(GFP_KERNEL);
		if (!_dev_name)
			goto out_free;
		strcpy((char*)_dev_name, dev_name);
	}

	if (type) {
		_type=__get_free_page(GFP_KERNEL);
		if (!_type)
			goto out_free;
		strcpy((char*)_type, type);
	}

	if (data) {
		_data=__get_free_page(GFP_KERNEL);
		if (!_data)
			goto out_free;
		strcpy((char*)_data, data);
	}

	err=sys_mount((char __user*)_dev_name, dir_name, (char __user*)_type,
		      flags, (char __user*)_data);

out_free:
	if (_dev_name)
		free_page(_dev_name);
	if (_type)
		free_page(_type);
	if (_data)
		free_page(_data);

	return err;
}


typedef long (*syscall_t)(long arg1, ...);

syscall_t syscall_table[NR_syscalls];

#define INIT_STE(x) syscall_table[__NR_##x]=(syscall_t)sys_##x;

void init_syscall_table(void)
{
	INIT_STE(ni_syscall);
	INIT_STE(sync);
	INIT_STE(reboot);
	INIT_STE(write);
	INIT_STE(close);
	INIT_STE(unlink);
	INIT_STE(open);
	INIT_STE(poll);
	INIT_STE(read);
	INIT_STE(lseek);
	INIT_STE(rename);
	INIT_STE(flock);
	INIT_STE(newfstat);
	INIT_STE(chmod);
	INIT_STE(newlstat);
	INIT_STE(mkdir);
	INIT_STE(rmdir);
	INIT_STE(getdents);
	INIT_STE(newstat);
	INIT_STE(utimes);
	INIT_STE(utime);
	INIT_STE(nanosleep);
	INIT_STE(mknod);
	INIT_STE(safe_mount);
	INIT_STE(umount);
	INIT_STE(chdir);
	INIT_STE(statfs);
	INIT_STE(chroot);
	INIT_STE(getcwd);
	INIT_STE(chown);
	INIT_STE(umask);
	INIT_STE(getuid);
	INIT_STE(getgid);
#ifdef CONFIG_NET
	INIT_STE(socketcall);
#endif
	INIT_STE(ioctl);
	INIT_STE(call);
}

struct syscall_req {
	int syscall;
	long params[6], ret;
	void *sem;
	struct list_head lh;
};


static LIST_HEAD(syscall_req_queue);
static spinlock_t syscall_req_lock=SPIN_LOCK_UNLOCKED;
DECLARE_WAIT_QUEUE_HEAD(syscall_req_wq);

static irqreturn_t syscall_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct syscall_req *sr=regs->irq_data;

	spin_lock(&syscall_req_lock);
	list_add(&sr->lh, &syscall_req_queue);
	spin_unlock(&syscall_req_lock);

	wake_up(&syscall_req_wq);

        return IRQ_HANDLED;
}

static struct syscall_req* dequeue_syscall_req(void)
{
	struct syscall_req *sr=NULL;
	spin_lock(&syscall_req_lock);
	if (!list_empty(&syscall_req_queue)) {
		sr=list_first_entry(&syscall_req_queue, typeof(*sr), lh);
		list_del(&sr->lh);
	}
	spin_unlock(&syscall_req_lock);

	return sr;
}

static inline struct syscall_req* linux_wait_syscall_request(void)
{
	struct syscall_req *sr;

	wait_event(syscall_req_wq, (sr=dequeue_syscall_req()) != NULL);

	return sr;
}

static inline long do_syscall(struct syscall_req *sr)
{
	int ret;

	if (sr->syscall < 0 || sr->syscall >= NR_syscalls || 
	    syscall_table[sr->syscall] == NULL)
		ret=-ENOSYS;
	else
		ret=syscall_table[sr->syscall](sr->params[0],
						   sr->params[1],
						   sr->params[2],
						   sr->params[3],
						   sr->params[4],
						   sr->params[5]);
	sr->ret=ret;
	linux_nops->sem_up(sr->sem);
	return ret;
}


int run_syscalls(void)
{
	snprintf(current->comm, sizeof(current->comm), "syscalld");
	while (1) {
		struct syscall_req *sr=linux_wait_syscall_request();
		if (sr->syscall == __NR_reboot) 
			break;
		do_syscall(sr);
	}

	return 0;
}

static struct irqaction irq1  = {
	.handler = syscall_irq,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING,
	.mask = CPU_MASK_CPU0,
        .dev_id = &irq1,
	.name = "syscall"
};

int __init syscall_init(void)
{
	init_syscall_table();
	setup_irq(1, &irq1);
	printk("lkl: syscall interface initialized\n");
	return 0;
}



/*
 * The system call stubs, provided for application convenince.
 */

#define SYSCALL_REQ(_syscall, _params...) \
	struct syscall_req sr = {	\
		.syscall = __NR_##_syscall,	\
		.params = { _params },		\
		.sem = linux_nops->sem_alloc(0), \
	};					\
	if (!sr.sem) \
		return -ENOMEM; \
	linux_trigger_irq_with_data(SYSCALL_IRQ, &sr); \
	linux_nops->sem_down(sr.sem); \
	linux_nops->sem_free(sr.sem); \
	return sr.ret; 


long lkl_sys_sync(void)
{
	SYSCALL_REQ(sync);
}

long lkl_sys_umount(const char *path, int flags)
{
	SYSCALL_REQ(umount, (long)path, flags);
}

ssize_t lkl_sys_write(unsigned int fd, const char *buf, size_t count)
{
	SYSCALL_REQ(write, fd, (long)buf, count);
}

long lkl_sys_close(unsigned int fd)
{
	SYSCALL_REQ(close, fd);
}

long lkl_sys_unlink(const char *pathname)
{
	SYSCALL_REQ(unlink, (long)pathname);
}

long lkl_sys_open(const char *filename, int flags, int mode)
{
       SYSCALL_REQ(open, (long)filename, flags, mode);
}

long lkl_sys_poll(struct pollfd *ufds, unsigned int nfds, long timeout)
{
	SYSCALL_REQ(poll, (long)ufds, nfds, timeout);
}

ssize_t lkl_sys_read(unsigned int fd, char *buf, size_t count)
{
	SYSCALL_REQ(read, fd, (long)buf, count);
}

off_t lkl_sys_lseek(unsigned int fd, off_t offset, unsigned int origin)
{
	SYSCALL_REQ(lseek, fd, offset, origin);
}

long lkl_sys_rename(const char *oldname, const char *newname)
{
	SYSCALL_REQ(rename, (long)oldname, (long)newname);
}

long lkl_sys_flock(unsigned int fd, unsigned int cmd)
{
	SYSCALL_REQ(flock, fd, cmd);
}

long lkl_sys_newfstat(unsigned int fd, struct stat *statbuf)
{
	SYSCALL_REQ(newfstat, fd, (long)statbuf);
}

long lkl_sys_chmod(const char *filename, mode_t mode)
{
	SYSCALL_REQ(chmod, (long)filename, mode);
}

long lkl_sys_newlstat(char *filename, struct stat *statbuf)
{
	SYSCALL_REQ(newlstat, (long)filename, (long)statbuf);
}

long lkl_sys_mkdir(const char *pathname, int mode)
{
	SYSCALL_REQ(mkdir, (long)pathname, mode);
}

long lkl_sys_rmdir(const char *pathname)
{
	SYSCALL_REQ(rmdir, (long)pathname);
}

long lkl_sys_getdents(unsigned int fd, struct linux_dirent *dirent, unsigned int count)
{
	SYSCALL_REQ(getdents, fd, (long)dirent, count);
}

long lkl_sys_newstat(char *filename, struct stat *statbuf)
{
	SYSCALL_REQ(newstat, (long)filename, (long)statbuf);
}

long lkl_sys_utimes(const char *filename, struct timeval *utimes)
{
	SYSCALL_REQ(utime, (long)filename, (long)utimes);
}

long lkl_sys_mount(const char *dev, const char *mnt_point, const char *fs, int flags, void *data)
{
	SYSCALL_REQ(safe_mount, (long)dev, (long)mnt_point, (long)fs, flags, (long)data);
}


long lkl_sys_chdir(const char *dir)
{
	SYSCALL_REQ(chdir, (long)dir);
}


long lkl_sys_mknod(const char *filename, int mode, unsigned dev)
{
	SYSCALL_REQ(mknod, (long)filename, mode, dev);
}


long lkl_sys_chroot(const char *dir)
{
	SYSCALL_REQ(chroot, (long)dir);
}

long lkl_sys_nanosleep(struct timespec *rqtp, struct timespec *rmtp)
{
	SYSCALL_REQ(nanosleep, (long)rqtp, (long)rmtp);
}

long lkl_sys_getcwd(char *buf, unsigned long size)
{
	SYSCALL_REQ(getcwd, (long)buf, (long) size);
}

long lkl_sys_utime(const char *filename, const struct utimbuf *buf)
{
        SYSCALL_REQ(utime, (long)filename, (long)buf);
}

long lkl_sys_socket(int family, int type, int protocol)
{
	long args[6]={family, type, protocol};
	SYSCALL_REQ(socketcall, SYS_SOCKET, (long)args);
}

long lkl_sys_send(int sock, void *buffer, size_t size, unsigned flags)
{
	long args[6]={sock, (long)buffer, size, flags};
	SYSCALL_REQ(socketcall, SYS_SEND, (long)args);
}

long lkl_sys_recv(int sock, void *buffer, size_t size, unsigned flags)
{
	long args[6]={sock, (long)buffer, size, flags};
	SYSCALL_REQ(socketcall, SYS_RECV, (long)args);
}


long lkl_sys_connect(int sock, struct sockaddr *saddr, int len)
{
	long args[6]={sock, (long)saddr, len};
	SYSCALL_REQ(socketcall, SYS_CONNECT, (long)args);
}

long lkl_sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	SYSCALL_REQ(ioctl, fd, cmd, arg);
}

long lkl_sys_umask(int mask)
{
	SYSCALL_REQ(umask, mask);
}

long lkl_sys_getuid(void)
{
	SYSCALL_REQ(getuid);
}

long lkl_sys_getgid(void)
{
	SYSCALL_REQ(getgid);
}

long sys_call(long _f, long arg1, long arg2, long arg3, long arg4, long arg5)
{
	long (*f)(long arg1, long arg2, long arg3, long arg4, long arg5)=
		(long (*)(long, long, long, long, long))_f;
	return f(arg1, arg2, arg3, arg4, arg5);
}

long lkl_sys_call(long f, long arg1, long arg2, long arg3, long arg4, long arg5)
{
	SYSCALL_REQ(call, f, arg1, arg2, arg3, arg4, arg5);
}

long lkl_sys_statfs(const char *path, struct statfs *buf)
{
	SYSCALL_REQ(statfs, (long)path, (long)buf);
}


/* 
 * Halt is special as we want to call syscall_done after the kernel has been
 * halted, in linux_start_kernel. 
 */
void *halt_syscall_sem;

long lkl_sys_halt(void)
{
	struct syscall_req sr = {	
		.syscall = __NR_reboot,
	};
	halt_syscall_sem=sr.sem=linux_nops->sem_alloc(0);
	linux_trigger_irq_with_data(SYSCALL_IRQ, &sr);
	linux_nops->sem_down(sr.sem);
	return sr.ret;
}     


late_initcall(syscall_init);
