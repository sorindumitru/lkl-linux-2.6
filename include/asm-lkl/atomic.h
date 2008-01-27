#ifndef _ASM_LKL_ATOMIC_H
#define _ASM_LKL_ATOMIC_H

#include <asm/system.h>

typedef int atomic_t;

#define ATOMIC_INIT(i) (i)

static __inline__ int atomic_read(const atomic_t *v)
{
	return  *v;
}

static __inline__ void atomic_set(atomic_t *v, int i)
{
	*v=i;
}

static __inline__ void atomic_add(int i, atomic_t *v)
{
	*v+=i;
}

static __inline__ void atomic_inc(atomic_t *v)
{
	return atomic_add(1, v);
}

static __inline__ void atomic_sub(int i, atomic_t *v)
{
	return atomic_add(-i, v);
}

static __inline__ void atomic_dec(atomic_t *v)
{
	return atomic_add(-1, v);
}

static __inline__ int atomic_add_and_test(int i, atomic_t *v)
{
	int ret;
	
	*v+=i;
	ret=(*v == 0);
	
	return ret;
}

static __inline__ int atomic_sub_and_test(int i, atomic_t *v)
{
	return atomic_add_and_test(-i, v);
}

static __inline__ int atomic_dec_and_test(atomic_t *v)
{
	return atomic_add_and_test(-1, v);
}

static __inline__ int atomic_inc_and_test(atomic_t *v)
{
	return atomic_add_and_test(1, v);
}

static __inline__ int atomic_add_negative(int i, atomic_t *v)
{
	int ret;
	
	*v+=i;
	ret=(*v < 0);
	
	return ret;
}

static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	int ret;
	
	*v+=i;
	ret=*v;
	
	return ret;
}

static __inline__ int atomic_sub_return(int i, atomic_t *v)
{
	return atomic_add_return(-i,v);
}

static __inline__ int atomic_inc_return(atomic_t *v)
{
	return atomic_add_return(1,v);
}

static __inline__ int atomic_dec_return(atomic_t *v)
{
	return atomic_add_return(-1,v);
}

static inline int atomic_sub_if_positive(int i, atomic_t *v)
{
	int ret;
	
	ret=*v - i;
	if (*v >= i)
	    *v -= i;
	return ret;
}

static __inline__ int atomic_dec_if_positive(atomic_t *v)
{
	return atomic_sub_if_positive(1, v);
}

/**
 * atomic_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static __inline__ int atomic_add_unless(atomic_t *v, int a, int u)
{
	int ret=1;
	
        if (*v != u) 
                *v+=a;
        else
                ret=0;
	return ret;
}

static __inline__ int atomic_inc_not_zero(atomic_t *v)
{
        return atomic_add_unless(v, 1, 0);
}

#define xchg(ptr, v) \
({ \
        __typeof__(*(ptr)) ov; \
        \
        ov=*ptr; \
        *ptr=v; \
        ov; \
})


#define atomic_xchg(v, new) (xchg(v, new))


#include <asm-generic/atomic.h>

#endif
