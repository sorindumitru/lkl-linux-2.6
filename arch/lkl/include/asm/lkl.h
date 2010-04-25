#ifndef _LKL_LKL_H
#define _LKL_LKL_H

#define __KERNEL_STRICT_NAMES

#include <linux/posix_types.h>
#include <asm/types.h>
#include <asm/stat.h>

/* horible hack, but i'll have it over hacks in apps */
#ifdef __GLIBC__
#define __LKL_GLIBC__ __GLIBC__
#undef __GLIBC__
#endif
#include <linux/stat.h>
#ifdef __LKL_GLIBC__
#define __GLIBC__ __LKL_GLIBC__
#undef __LKL_GLIBC__
#endif
#include <asm/statfs.h>
#include <linux/fcntl.h>

struct sockaddr;
struct pollfd;
struct msghdr;

/* 
 * FIXME: these structs are duplicated from within the kernel. Find a way
 * to remove duplicates. (maybe define them in arch??)
 *
 * For now, these must be kept in sync with their Linux counterparts. 
 */
struct __kernel_timespec {
	__kernel_time_t	tv_sec;
	long tv_nsec;
};
struct __kernel_timeval {
	__kernel_time_t tv_sec;
	__kernel_suseconds_t tv_usec;
};

struct __kernel_timezone {
	int	tz_minuteswest;	
	int	tz_dsttime;	
};

struct __kernel_utimbuf {
	__kernel_time_t actime;
	__kernel_time_t modtime;
};

#ifndef __KERNEL__
typedef __u32 __kernel_dev_t;
#endif

struct __kernel_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};

long lkl_sys_sync(void);
long lkl_sys_reboot(int magic1, int magic2, unsigned int cmd,  void *arg);
__kernel_ssize_t lkl_sys_write(unsigned int fd, const char *buf, __kernel_size_t count);
long lkl_sys_close(unsigned int fd);
long lkl_sys_unlink(const char *pathname);
long lkl_sys_open(const char *filename, int flags, int mode);
long lkl_sys_poll(struct pollfd *ufds, unsigned int nfds, long timeout);
__kernel_ssize_t lkl_sys_read(unsigned int fd, char *buf,  __kernel_size_t count);
__kernel_off_t lkl_sys_lseek(unsigned int fd, __kernel_off_t offset,  unsigned int origin);
long lkl_sys_rename(const char *oldname,  const char *newname);
long lkl_sys_flock(unsigned int fd, unsigned int cmd);
long lkl_sys_newfstat(unsigned int fd, struct __kernel_stat *statbuf);
long lkl_sys_chmod(const char *filename, __kernel_mode_t mode);
long lkl_sys_newlstat(char *filename, struct __kernel_stat *statbuf);
long lkl_sys_mkdir(const char *pathname, int mode);
long lkl_sys_rmdir(const char *pathname);
long lkl_sys_getdents(unsigned int fd, struct __kernel_dirent *dirent, unsigned int count);
long lkl_sys_newstat(char *filename, struct __kernel_stat *statbuf);
long lkl_sys_utimes(const char *filename, struct __kernel_timeval *utimes);
long lkl_sys_nanosleep(struct __kernel_timespec *rqtp, struct __kernel_timespec *rmtp);
long lkl_sys_mknod(const char *filename, int mode, unsigned dev);
long lkl_sys_umount(char *name, int flags);
long lkl_sys_chdir(const char *filename);
long lkl_sys_statfs(const char * path, struct __kernel_statfs *buf);
long lkl_sys_chroot(const char *filename);
long lkl_sys_mount(char *dev_name, char *dir_name,
		    char *type, unsigned long flags, void *data);
long lkl_sys_halt(void);
long lkl_sys_getcwd(char *buf, unsigned long size);
long lkl_sys_utime(const char *filename, const struct __kernel_utimbuf *buf);
long lkl_sys_socket(int family, int type, int protocol);
long lkl_sys_bind(int sock, const struct sockaddr *saddr, unsigned int addrlen);
long lkl_sys_connect(int sock, struct sockaddr *saddr, int len);
long lkl_sys_listen(int sock, int backlog);
long lkl_sys_accept(int sock, struct sockaddr *saddr, unsigned int* addrlen);
long lkl_sys_send(int sock, void *buffer, __kernel_size_t size, unsigned flags);
long lkl_sys_recv(int sock, void *buffer, __kernel_size_t size, unsigned flags);
long lkl_sys_sendmsg(int sockfd, struct msghdr *msg, int flags);
long lkl_sys_recvmsg(int sockfd, struct msghdr *msg, int flags);
long lkl_sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
long lkl_sys_connect(int sock, struct sockaddr *saddr, int len);
long lkl_sys_setsockopt(int sockfd, int level, int optname,const void *optval, int optlen);
long lkl_sys_getsockname(int sockfd, struct sockaddr *addr, int *addrlen);
long lkl_sys_umask(int mask);
long lkl_sys_getuid(void);
long lkl_sys_getgid(void);
long lkl_sys_call(long f, long arg1, long arg2, long arg3, long arg4, long arg5);
long lkl_sys_access(const char *filename, int mode);
long lkl_sys_truncate(const char *path, unsigned long length);
__kernel_ssize_t lkl_sys_pwrite64(unsigned int fd, const char *buf,
				  __kernel_size_t count, __kernel_loff_t pos);
__kernel_ssize_t lkl_sys_pread64(unsigned int fd, char *buf,
				 __kernel_size_t count, __kernel_loff_t pos);


int sprintf(char * buf, const char * fmt,
	    ...) __attribute__ ((format (printf, 2, 3)));
int snprintf(char * buf, __kernel_size_t size, const char * fmt,
	     ...) __attribute__ ((format (printf, 3, 4)));
int sscanf(const char *, const char *,
	   ...) __attribute__ ((format (scanf, 2, 3)));


#ifndef strchr
extern char * strchr(const char *,int);
#endif /* strchr */

#ifndef strlen
extern __kernel_size_t strlen(const char *);
#endif /* strlen */

/*
 * All subsequent system calls from the current native thread will be executed
 * in a newly created Linux system call thread. System calls are still
 * serialized during execution, but this allows running other system calls while
 * one is blocked, if the application is multithreaded.
 */
int lkl_syscall_thread_init(void);

/*
 * After this point, all system calls issued by this thread will be executed by
 * the default Linux system call thread, as before calling
 * lkl_syscall_thread_init().
 */
int lkl_syscall_thread_cleanup(void);

#endif
