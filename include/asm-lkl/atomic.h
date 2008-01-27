#ifndef _ASM_LKL_ATOMIC_H
#define _ASM_LKL_ATOMIC_H

#include <asm/system.h>

typedef long atomic_t;

#define ATOMIC_INIT(i) (i)

static __inline__ long atomic_read(const atomic_t *v)
{
	return  *v;
}

static __inline__ void atomic_set(atomic_t *v, long l)
{
	*v=l;
}

static __inline__ void atomic_add(long l, atomic_t *v)
{
	*v+=l;
}

static __inline__ void atomic_inc(atomic_t *v)
{
	return atomic_add(1, v);
}

static __inline__ void atomic_sub(long l, atomic_t *v)
{
	return atomic_add(-l, v);
}

static __inline__ void atomic_dec(atomic_t *v)
{
	return atomic_add(-1, v);
}

static __inline__ long atomic_add_and_test(long l, atomic_t *v)
{
	long ret;
	
	*v+=l;
	ret=(*v == 0);
	
	return ret;
}

static __inline__ long atomic_sub_and_test(long l, atomic_t *v)
{
	return atomic_add_and_test(-l, v);
}

static __inline__ long atomic_dec_and_test(atomic_t *v)
{
	return atomic_add_and_test(-1, v);
}

static __inline__ long atomic_inc_and_test(atomic_t *v)
{
	return atomic_add_and_test(1, v);
}

static __inline__ long atomic_add_negative(long l, atomic_t *v)
{
	long ret;
	
	*v+=l;
	ret=(*v < 0);
	
	return ret;
}

static __inline__ long atomic_add_return(long l, atomic_t *v)
{
	long ret;
	
	*v+=l;
	ret=*v;
	
	return ret;
}

static __inline__ long atomic_sub_return(long l, atomic_t *v)
{
	return atomic_add_return(-l,v);
}

static __inline__ long atomic_inc_return(atomic_t *v)
{
	return atomic_add_return(1,v);
}

static __inline__ long atomic_dec_return(atomic_t *v)
{
	return atomic_add_return(-1,v);
}

static inline long atomic_sub_if_positive(long l, atomic_t *v)
{
	long ret;
	
	ret=*v - l;
	if (*v >= l)
	    *v -= l;
	return ret;
}

static __inline__ long atomic_dec_if_positive(atomic_t *v)
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
static __inline__ long atomic_add_unless(atomic_t *v, long a, long u)
{
	long ret=1;
	
        if (*v != u) 
                *v+=a;
        else
                ret=0;
	return ret;
}

static __inline__ long atomic_inc_not_zero(atomic_t *v)
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
