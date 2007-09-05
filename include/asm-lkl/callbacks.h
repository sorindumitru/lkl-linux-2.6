#ifndef _ASM_LKL_CALLBACKS_H
#define _ASM_LKL_CALLBACKS_H

struct linux_native_operations {
	/*
	 * Called during a kernel panic. 	 
	 */
	long (*panic_blink)(long time);

	/*
	 * The size of the application thread info.
	 */
	int thread_info_size;

	/*
	 * Initialize the application thread info.
	 */
	void (*thread_info_init)(void *thread_data);

	/*
	 * Create a new stopped thread and arrange it to run f(arg) when
	 * started (via linux_switch_to). The application can use the
	 * thread_info area to store information about this thread.
	 */
	int (*new_thread)(int (*f)(void*), void *arg, void *thread_info);


	/*
	 * Perform a context switch: the current thread (described by the prev
	 * application thread info) should be stopped, and the thread
	 * described by the next application thread info should be started.
	 */
	void (*switch_to)(void *prev, void *next);

	/*
	 * Free the thread described by thread_info.
	 */
	void (*free_thread)(void *thread_info);

	/*
	 * Allocate the memory area to be used by the linux kernel. Store the
	 * start address in start and the size in size. Internally, the start
	 * and size will be rounded to conform with page boundaries. 
	 */
	void (*mem_init)(unsigned long *start, unsigned long *size);

	/*
	 * This routine is called after the kernel initialization is
	 * complete. It should never exit. 
	 */
	void (*main)(void);

	/*
	 * Semaphore routines to create, destroy increment and
	 * decrement (and possible block the current thread, and whole
	 * kernel for that matter)the semphore count. 
	 */
	void* (*new_sem)(int count);

	void (*sem_up)(void *sem);

	void (*sem_down)(void *sem);

	void (*free_sem)(void *sem);

};

extern struct linux_native_operations *linux_nops;

/*
 * Signal an interrupt. data will be passed in irq_data of the pt_regs
 * structure. (see get_irq_regs)
 */
int linux_trigger_irq(int irq, void *data);

/*
 * Register the native operations and start the kernel.
 */
int linux_start_kernel(struct linux_native_operations *lnops, const char *cmd_line, ...);

#endif
