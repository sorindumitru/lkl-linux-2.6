#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <asm/irq_regs.h>
#include <linux/sched.h>

#include <asm/callbacks.h>

unsigned long irqs_enabled=0;

unsigned long __local_save_flags(void)
{
        return irqs_enabled;
}

void __local_irq_restore(unsigned long flags)
{
        irqs_enabled=flags;
}

void local_irq_enable(void)
{
        irqs_enabled=1;
}

void local_irq_disable(void)
{
        irqs_enabled=0;
}


/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */
unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	__do_IRQ(irq);
	irq_exit();
	set_irq_regs(old_regs);
	return 1;
}

struct irq_shot {
	struct list_head list;
	struct pt_regs regs;
};

struct list_head irq_shots[NR_IRQS];

void *irq_sem;

int linux_trigger_irq(int irq, void *data)
{
	struct irq_shot *is=kmalloc(sizeof(*is), GFP_ATOMIC);

	if (!is)
		return -ENOMEM;

	is->regs.irq_data=data;
	list_add_tail(&is->list, &irq_shots[irq]);

	linux_sem_up(irq_sem);

	return 0;
}


int dequeue_irq(int irq, struct pt_regs *regs)
{
	struct list_head *i;
	struct irq_shot *is=NULL;

	list_for_each(i, &irq_shots[irq]) {
		is=list_entry(i, struct irq_shot, list);
		break;
	}

	if (!is)
		return -ENOENT;

	list_del(&is->list);
	*regs=is->regs;
	kfree(is);

	return 0;
}


/*
 * We run the IRQ handlers from here so that we don't have to
 * interrupt the current thread since the application might not be
 * able to do that.
 */
void cpu_idle(void)
{
	while (1) {
		struct pt_regs regs;
		int i;

		do {
			linux_sem_down(irq_sem);
			for(i=0; i<NR_IRQS; i++)
				if (dequeue_irq(i, &regs) == 0) 
					do_IRQ(i, &regs);
		} while (!need_resched());

		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}

void init_IRQ(void)
{
	int i;
	
	for(i=0; i<NR_IRQS; i++) {
		INIT_LIST_HEAD(&irq_shots[i]);
		set_irq_chip_and_handler(i, &dummy_irq_chip, handle_simple_irq);
	}
	irq_sem=linux_sem_new(0);
	BUG_ON(irq_sem == NULL);
}
