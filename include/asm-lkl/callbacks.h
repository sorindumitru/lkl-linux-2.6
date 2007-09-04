#ifndef _ASM_LKL_CALLBACKS_H
#define _ASM_LKL_CALLBACKS_H

/*
 * Returns a pointer to a string to be used as a command line. The
 * storage buffer should be writable and _will_ be modified.
 */
extern void linux_cmd_line(char **cmd_line);

/*
 * Called during a kernel panic. 
 */
extern long linux_panic_blink(long time);


/*
 * The size of the application thread info.
 */
extern int linux_thread_info_size;

/*
 * Initialize the application thread info.
 */
extern void linux_thread_info_init(void *thread_data);

/*
 * Create a new stopped thread and arrange it to run f(arg) when
 * started (via linux_switch_to). The application can use the
 * thread_info area to store information about this thread.
 */
extern int linux_new_thread(int (*f)(void*), void *arg, void *thread_info);


/*
 * Perform a context switch: the current thread (described by the prev
 * application thread info) should be stopped, and the thread
 * described by the next application thread info should be started.
 */
extern void linux_switch_to(void *prev, void *next);

/*
 * Free the thread described by thread_info.
 */
extern void linux_free_thread(void *thread_info);

/*
 * Allocate the memory area to be used by the linux kernel. Store the
 * start address in start and the size in size. Internally, the start
 * and size will be rounded to conform with page boundaries. 
 */
extern void linux_mem_init(unsigned long *start, unsigned long *size);

/*
 * This routine is called after the kernel initialization is
 * complete. It should never exit. 
 */
extern void linux_main(void);

/*
 * Create a new semaphore and return a handle to it, to be used with
 * the linux_sem_up and linux_sem_down functions.
 */
extern void* linux_sem_new(int count);

extern void linux_sem_up(void *sem);
extern void linux_sem_down(void *sem);


/*
 * Signal an interrupt. data will be passed in irq_data of the pt_regs
 * structure. (see get_irq_regs)
 */
int linux_trigger_irq(int irq, void *data);

#endif
