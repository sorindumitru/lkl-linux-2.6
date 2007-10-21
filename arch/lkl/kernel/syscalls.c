#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/callbacks.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/syscalls.h>

/*
 * sys_mount (is sloppy?? and) copies a full page from dev_name, type and
 * data. This can trigger page faults (which a normal trigger would
 * ignore). 
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
	INIT_STE(nanosleep);
	INIT_STE(mknod);
	INIT_STE(safe_mount);
	INIT_STE(umount);
	INIT_STE(chdir);
	INIT_STE(statfs);
	INIT_STE(chroot);
}

struct _syscall_req {
	int syscall;
	long params[6], ret;
	void *data;
	void (*done)(void*);
	struct list_head lh;
};


long linux_syscall(struct syscall_req *sr)
{
	int ret;

	if (sr->syscall < 0 || sr->syscall >= NR_syscalls)
		ret=-ENOSYS;
	else
		ret=syscall_table[sr->syscall](sr->params[0],
						   sr->params[1],
						   sr->params[2],
						   sr->params[3],
						   sr->params[4],
						   sr->params[5]);
	sr->ret=ret;
	sr->done(sr->data);
	return ret;
}

static LIST_HEAD(syscall_req_queue);
static spinlock_t syscall_req_lock=SPIN_LOCK_UNLOCKED;
DECLARE_WAIT_QUEUE_HEAD(syscall_req_wq);

static irqreturn_t syscall_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct _syscall_req *sr=regs->irq_data;

	spin_lock(&syscall_req_lock);
	list_add(&sr->lh, &syscall_req_queue);
	spin_unlock(&syscall_req_lock);

	wake_up(&syscall_req_wq);

        return IRQ_HANDLED;
}

static struct _syscall_req* dequeue_syscall_req(void)
{
	struct _syscall_req *sr=NULL;
	spin_lock(&syscall_req_lock);
	if (!list_empty(&syscall_req_queue)) {
		sr=list_first_entry(&syscall_req_queue, typeof(*sr), lh);
		list_del(&sr->lh);
	}
	spin_unlock(&syscall_req_lock);

	return sr;
}

struct syscall_req* linux_wait_syscall_request(void)
{
	struct _syscall_req *sr;

	wait_event(syscall_req_wq, (sr=dequeue_syscall_req()) != NULL);

	return (struct syscall_req*)sr;
}


static struct irqaction irq1  = {
	.handler = syscall_irq,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING,
	.mask = CPU_MASK_CPU0,
        .dev_id = &linux_syscall,
	.name = "syscall"
};

int __init syscall_init(void)
{
	init_syscall_table();
	setup_irq(1, &irq1);
	printk("lkl: syscall interface initialized\n");
	return 0;
}

late_initcall(syscall_init);
