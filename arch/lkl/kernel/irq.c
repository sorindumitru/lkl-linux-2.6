#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <asm/irq_regs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/tick.h>

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


unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	__do_IRQ(irq);
	irq_exit();
	set_irq_regs(old_regs);
	return 1;
}

struct irq_data {
	struct list_head list;
	struct pt_regs regs;
};

struct irq_shot {
        struct list_head data_list;
        int no_data_count;
} irq_shots[NR_IRQS];

void linux_trigger_irq(int irq)
{
	BUG_ON(linux_nops->exit_idle == NULL);

        irq_shots[irq].no_data_count++;

        linux_nops->exit_idle();
}

int linux_trigger_irq_with_data(int irq, void *data)
{
	struct irq_data *is;

	BUG_ON(linux_nops->exit_idle == NULL);

	if (!(is=kmalloc(sizeof(*is), GFP_ATOMIC)))
		return -ENOMEM;

	is->regs.irq_data=data;
	list_add_tail(&is->list, &irq_shots[irq].data_list);

        linux_nops->exit_idle();

	return 0;
}


static int dequeue_data(int irq, struct pt_regs *regs)
{
	struct list_head *i;
	struct irq_data *is=NULL;

	list_for_each(i, &irq_shots[irq].data_list) {
		is=list_entry(i, struct irq_data, list);
		break;
	}

	if (!is)
		return -ENOENT;

	list_del(&is->list);
	*regs=is->regs;
	kfree(is);

	return 0;
}

static int dequeue_nodata(int irq, struct pt_regs *regs)
{
        if (irq_shots[irq].no_data_count <= 0)
                return -ENOENT;
        
        irq_shots[irq].no_data_count--;
        return 0;
}

extern int linux_halted;

void run_irqs(void)
{
        local_irq_disable();

        do {
		struct pt_regs regs;
		int i;

		linux_nops->enter_idle(linux_halted);

                for(i=0; i<NR_IRQS; i++) {
                        while (dequeue_nodata(i, &regs) == 0)
                                do_IRQ(i, &regs);
                        while (dequeue_data(i, &regs) == 0) 
                                do_IRQ(i, &regs);
                }
        } while (!need_resched() && !linux_halted);

        local_irq_enable();
}


static struct rcu_head rcu_head;
static int rcu_done, rcu_start;
static void rcu_func(struct rcu_head *h)
{
       rcu_done=1;
}


/*
 * We run the IRQ handlers from here so that we don't have to
 * interrupt the current thread since the application might not be
 * able to do that.
 */
void cpu_idle(void)
{
	int loop=1;

	BUG_ON(linux_nops->enter_idle == NULL);

	while (loop) {
		/*
		 * We need to exit, but before we can do so we need to run any
		 * pending jobs, like RCU calls, IRQs, or runable processes.
		 */
		if (linux_halted) {
			if (!rcu_start) {
				call_rcu(&rcu_head, rcu_func);
				rcu_start=1;
			}
			loop--;
		}
		tick_nohz_stop_sched_tick();		
		run_irqs();
		if (need_resched() || !rcu_done) {
			/* 
			 * We have either runable processes or RCU calls to 
			 * process. Don't exit this turn. Maybe next time.
			 */ 
			if (linux_halted)
				loop++;
			tick_nohz_restart_sched_tick();
			preempt_enable_no_resched();
			schedule();
			preempt_disable();
		}

		
	}

}

void init_IRQ(void)
{
	int i;
	
	if (!linux_nops->enter_idle || !linux_nops->exit_idle)
		return;

	for(i=0; i<NR_IRQS; i++) {
		INIT_LIST_HEAD(&irq_shots[i].data_list);
		set_irq_chip_and_handler(i, &dummy_irq_chip, handle_simple_irq);
	}

	printk(KERN_INFO "lkl: with IRQ support\n");
}


int show_interrupts(struct seq_file *p, void *v)
{
        return 0;
}


void __init trap_init(void)
{
}
