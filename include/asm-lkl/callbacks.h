#ifndef _ASM_LKL_CALLBACKS_H
#define _ASM_LKL_CALLBACKS_H

#include <asm/irq.h>

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
	 * started (via context_switch). The application can use the
	 * thread_info area to store information about this thread.
	 */
	int (*new_thread)(int (*f)(void*), void *arg, void *thread_info);


	/*
	 * Perform a context switch: the current thread (described by the prev
	 * application thread info) should be stopped, and the thread
	 * described by the next application thread info should be started.
	 */
	void (*context_switch)(void *prev, void *next);

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
	 * complete. When this function returns, the linux kernel will shutdown,
	 * and eventually the linux_start_kernel function will return.
	 */
	void (*main)(void);

	/*
	 * If no exit_idle operations are pending, block until we receive such
	 * an operation. This is called by the linux kernel when there is no
	 * work to be done.
	 */
	void (*enter_idle)(void);

	/*
	 * Unblock the enter_idle operation. This will resume the linux kernel
	 * and will usually be called from linux_trigger_irq() routines. 
	 */
	void (*exit_idle)(void);

        /*
         * Request a timer interrupt in delta nanoseconds, i.e. a
         * linux_trigger_irq(TIMER_IRQ)  call. If delta is zero, cancel
         * the pending timer. If delta is not zero and a previous timer was
         * armed, cancel the previous one.
         */
	void (*timer)(unsigned long delta);
        
        /*
         * Return the current time in nanoseconds 
         */
        unsigned long long (*time)(void);

	/*
	 * Kernel has been halted, linux_start_kernel will exit soon. This is
	 * the chance for the app to clean-up any resources allocated so far. Do
	 * not forget to:
	 *
	 * - free the  memory allocated via mem_init
	 * - stop any pending timers?
	 * 
	 */
	void (*halt)(void);
};

extern struct linux_native_operations *linux_nops;

/*
 * Signal an interrupt. Can be called at any time, including early boot time. 
 */
void linux_trigger_irq(int irq);


/*
 * Signal an interrupt with data to be passed in irq_data of the pt_regs
 * structure. (see get_irq_regs). For device driver convenience. Can't be called
 * at during early boot time, before the SLAB is initialized.
 */
int linux_trigger_irq_with_data(int irq, void *data);



/*
 * Register the native operations and start the kernel. The function returns
 * only after the kernel shutdowns. 
 */
int linux_start_kernel(struct linux_native_operations *lnops, const char *cmd_line, ...);

#endif
