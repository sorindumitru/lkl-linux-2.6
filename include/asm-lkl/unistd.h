#ifndef _ASM_LKL_UNISTD_H
#define _ASM_LKL_UNISTD_H

/*
 * FIXME: properly defined all __NR_ stuff and remove this.
 */
#include <asm-i386/unistd.h>

#undef __ARCH_WANT_SYS_SIGPROCMASK
#undef __ARCH_WANT_OLD_STAT

#define __NR_safe_mount __NR_mount 
#define __NR_newstat __NR_stat
#define __NR_newfstat __NR_fstat
#define __NR_newlstat __NR_lstat



#endif
