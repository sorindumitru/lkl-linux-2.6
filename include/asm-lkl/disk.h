#ifndef _LKL_DISK_H
#define _LKL_DISK_H

#ifndef __KERNEL__
#include <asm/lkl.h>
#endif

#define LKL_DISK_CS_ERROR 0
#define LKL_DISK_CS_SUCCESS 1

struct lkl_disk_cs {
	void *linux_cookie;
	int status, sync;
};
void lkl_disk_do_rw(void *f, unsigned long sector, unsigned long nsect,
		    char *buffer, int dir, struct lkl_disk_cs *cs);

__kernel_dev_t _lkl_disk_add_disk(void *data, int sectors);
int _lkl_disk_del_disk(__kernel_dev_t dev);

/*
 * These are called by the application and have to run in Linux context. Be nice
 * and provide the wrappers here.
 */ 
#ifndef __KERNEL__
static inline int lkl_disk_del_disk(__kernel_dev_t dev)
{
	return lkl_sys_call((long)_lkl_disk_del_disk, (long)dev, 0, 0, 0, 0);
}

static inline __kernel_dev_t lkl_disk_add_disk(void *data, int sectors)
{
	return lkl_sys_call((long)_lkl_disk_add_disk, (long)data,
			    (long)sectors, 0, 0, 0);
}
#endif

#endif
