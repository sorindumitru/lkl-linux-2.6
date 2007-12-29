#ifndef _ASM_LKL_IRQ_H
#define _ASM_LKL_IRQ_H

#define TIMER_IRQ 0
#define SYSCALL_IRQ 1
#define DISK_IRQ 2
#define ETH_IRQ 3

#ifndef NR_IRQS 
#define NR_IRQS 4
#else
#if NR_IRQS < 4
#error too few interrupts 
#endif
#endif

#endif
