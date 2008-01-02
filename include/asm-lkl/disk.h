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

__kernel_dev_t lkl_disk_add_disk(void *data, int sectors);
int lkl_disk_del_disk(__kernel_dev_t dev);

#endif
