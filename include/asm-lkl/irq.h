#ifndef _ASM_LKL_IRQ_H
#define _ASM_LKL_IRQ_H

#define TIMER_IRQ 0
#define SYSCALL_IRQ 1

#ifndef NR_IRQS
#define NR_IRQS 2
#else
#if NR_IRQS < 2
#error too few interrupts 
#endif
#endif

#endif
