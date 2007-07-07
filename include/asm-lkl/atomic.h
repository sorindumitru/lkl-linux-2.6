#ifndef _ASM_LKL_ATOMIC_H
#define _ASM_LKL_ATOMIC_H

typedef struct { int counter; } atomic_t;

#define ATOMIC_INIT(i) { (i) }

#define atomic_read(v) ((v)->counter)
#define atomic_set(v,i) ((v)->counter=(i))

static __inline__ void atomic_add(int i, atomic_t *v)
{
	long flags;
	
	local_irq_save(flags);
	v->counter+=i;
	local_irq_restore(flags);
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
	long flags;
	int ret;
	
	local_irq_save(flags);
	v->counter+=i;
	ret=(v->counter == 0);
	local_irq_restore(flags);
	
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
	long flags;
	int ret;
	
	local_irq_save(flags);
	v->counter+=i;
	ret=(v->counter < 0);
	local_irq_restore(flags);
	
	return ret;
}

static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	long flags;
	int ret;
	
	local_irq_save(flags);
	v->counter+=i;
	ret=v->counter;
	local_irq_restore(flags);
	
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
	long flags;
	int ret;
	
	local_irq_save(flags);
	ret=v->counter - i;
	if (v->counter >= i)
	    v->counter -= i;
	local_irq_restore(flags);
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
	long flags;
	int ret=1;
	
	local_irq_save(flags);
        if (v->counter != u) 
                v->counter+=a;
        else
                ret=0;
	local_irq_restore(flags);
	return ret;
}

static __inline__ int atomic_inc_not_zero(atomic_t *v)
{
        return atomic_add_unless(v, 1, 0);
}

#define xchg(ptr, v) \
({ \
        __typeof__(*(ptr)) ov; \
        long flags; \
        \
        local_irq_save(flags); \
        ov=*ptr; \
        *ptr=v; \
        local_irq_restore(flags); \
        ov; \
})


#define atomic_cmpxchg(v, old, new) ((int)cmpxchg(&((v)->counter), old, new))
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))


#include <asm-generic/atomic.h>

#endif
