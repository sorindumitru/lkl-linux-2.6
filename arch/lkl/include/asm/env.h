#ifndef _LKL_ENV_H
#define _LKL_ENV_H

#include <asm/lkl.h>

int lkl_env_init(unsigned long mem_size);

long lkl_mount(char *dev, char *mnt, int flags, void *data);
long lkl_mount_dev(__kernel_dev_t dev, char *fs_type, int flags, void *data,
		   char *mnt_str, int mnt_str_len);
long lkl_umount_dev(__kernel_dev_t dev, int flags);


int lkl_if_up(int ifindex);
int lkl_if_down(int ifindex);
int lkl_if_set_ipv4(int ifindex, unsigned int addr, unsigned int netmask_len);
int lkl_set_gateway(unsigned int addr);

int lkl_printf(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));


#endif
