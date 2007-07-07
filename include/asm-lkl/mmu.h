#ifndef _ASM_LKL_MMU_H
#define _ASM_LKL_MMU_H

typedef struct {
	struct vm_list_struct	*vmlist;
	unsigned long		end_brk;
} mm_context_t;

struct mm_struct;

#define HAVE_ARCH_PICK_MMAP_LAYOUT
static inline void arch_pick_mmap_layout(struct mm_struct *mm)
{
}

#define TASK_SIZE	(PAGE_OFFSET)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	0xdead0020

#endif
