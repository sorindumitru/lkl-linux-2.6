#ifndef _ASM_LKL_UNISTD_H
#define _ASM_LKL_UNISTD_H


#define cond_syscall(x) 

#include <asm-i386/unistd.h>

#undef __ARCH_WANT_SYS_SIGPROCMASK

#ifndef __KERNEL__

#include <asm/linkage.h>
#undef __KERNEL_STRICT_NAMES
#include <linux/types.h>

#include <asm/poll.h>
#include <asm/stat.h>
#include <asm/fcntl.h>
#include <linux/time.h>
#include <asm-generic/statfs.h>
#include <linux/stat.h>

struct linux_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};

long lkl_sys_sync(void);
long lkl_sys_reboot(int magic1, int magic2, unsigned int cmd,  void __user *arg);
ssize_t lkl_sys_write(unsigned int fd, const char __user *buf,    size_t count);
long lkl_sys_close(unsigned int fd);
long lkl_sys_unlink(const char __user *pathname);
long lkl_sys_open(const char __user *filename, int flags, int mode);
long lkl_sys_poll(struct pollfd __user *ufds, unsigned int nfds, long timeout);
ssize_t lkl_sys_read(unsigned int fd, char __user *buf,  size_t count);
off_t lkl_sys_lseek(unsigned int fd, off_t offset,  unsigned int origin);
long lkl_sys_rename(const char __user *oldname,  const char __user *newname);
long lkl_sys_flock(unsigned int fd, unsigned int cmd);
long lkl_sys_newfstat(unsigned int fd, struct stat __user *statbuf);
long lkl_sys_chmod(const char __user *filename, mode_t mode);
long lkl_sys_newlstat(char __user *filename, struct stat __user *statbuf);
long lkl_sys_mkdir(const char __user *pathname, int mode);
long lkl_sys_rmdir(const char __user *pathname);
long lkl_sys_getdents(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count);
long lkl_sys_newstat(char __user *filename, struct stat __user *statbuf);
long lkl_sys_utimes(const char __user *filename, struct timeval __user *utimes);
long lkl_sys_nanosleep(struct timespec __user *rqtp, struct timespec __user *rmtp);
long lkl_sys_mknod(const char __user *filename, int mode, unsigned dev);
long lkl_sys_umount(char __user *name, int flags);
long lkl_sys_chdir(const char __user *filename);
long lkl_sys_statfs(const char __user * path, struct statfs __user *buf);
long lkl_sys_chroot(const char __user *filename);
long lkl_sys_mount(char __user *dev_name, char __user *dir_name,
		    char __user *type, unsigned long flags, void __user *data);
long lkl_sys_halt(void);

int sprintf(char * buf, const char * fmt,
	    ...) __attribute__ ((format (printf, 2, 3)));
int snprintf(char * buf, size_t size, const char * fmt,
	     ...) __attribute__ ((format (printf, 3, 4)));
int sscanf(const char *, const char *,
	   ...) __attribute__ ((format (scanf, 2, 3)));

#endif 

#define __NR_safe_mount __NR_mount 
#define __NR_newstat __NR_stat
#define __NR_newfstat __NR_fstat
#define __NR_newlstat __NR_lstat


#endif
