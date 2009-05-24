#ifndef _ASM_LKL_UNALIGNED_H
#define _ASM_LKL_UNALIGNED_H

#define get_unaligned(ptr) (*(ptr))
#define put_unaligned(val, ptr) ((void)( *(ptr) = (val) ))


#endif
