#ifndef _ASM_LKL_CACHE_H
#define _ASM_LKL_CACHE_H

#include <asm/smp.h>
#include <asm/target.h>

#define rmb() mb()
#define wmb() mb()

//FIXME: need to get this from somewhere or make it configurable
#define L1_CACHE_BYTES 128


#define ____cacheline_aligned


#endif
