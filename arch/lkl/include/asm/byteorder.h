#ifndef _ASM_LKL_BYTEORDER_H
#define _ASM_LKL_BYTEORDER_H


#if defined(LITTLE_ENDIAN)
#  include <linux/byteorder/little_endian.h>
#elif  defined(BIG_ENDIAN)
#  include <linux/byteorder/big_endian.h>
#else
#  error unknown endianess
#endif

#endif//_ASM_LKL_BYTEORDER_H
