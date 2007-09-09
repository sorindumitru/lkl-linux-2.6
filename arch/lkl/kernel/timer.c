#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/callbacks.h>

void __udelay(unsigned long usecs)
{
}

void __ndelay(unsigned long nsecs)
{
}

void __devinit calibrate_delay(void)
{
}

static cycle_t clock_read(void)
{
        return linux_nops->time();
}

static struct clocksource clocksource = {
	.name	= "lkl",
	.rating = 499,
	.read	= clock_read,
        .mult   = 1, 
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.flags  = CLOCK_SOURCE_IS_CONTINUOUS
};

static void clockevent_set_mode(enum clock_event_mode mode,
                                struct clock_event_device *evt)
{
	switch(mode) {
	default:
	case CLOCK_EVT_MODE_PERIODIC:
		BUG();
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
		linux_nops->timer(0);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
                break;
	}
}

static irqreturn_t timer_irq(int irq, void *dev_id)
{
        struct clock_event_device *dev=(struct clock_event_device*)dev_id;

        dev->event_handler(dev);
        
        return IRQ_HANDLED;
}
  
static int clockevent_next_event(unsigned long delta,
                                 struct clock_event_device *evt) 
{
        if (delta !=0 )
                linux_nops->timer(delta);
        else
                linux_trigger_irq(TIMER_IRQ);

        return 0;
}

static struct clock_event_device clockevent = {
	.name		= "lkl",
	.features	= CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= clockevent_set_mode,
	.set_next_event = clockevent_next_event,
	.shift		= 0,
        .mult           = 1,
	.irq		= 0,
        .cpumask        = CPU_MASK_CPU0,
        .max_delta_ns   = 0xffffffff,
        .min_delta_ns   = 0
};


static struct irqaction irq0  = {
	.handler = timer_irq,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING | IRQF_TIMER,
	.mask = CPU_MASK_CPU0,
        .dev_id = &clockevent,
	.name = "timer"
};

void __init time_init(void)
{
        if (!linux_nops->enter_idle || !linux_nops->exit_idle || 
	    !linux_nops->timer || !linux_nops->time) 
		return;

	setup_irq(0, &irq0);

	BUG_ON(clocksource_register(&clocksource) != 0);
        
        clockevents_register_device(&clockevent);

        printk("lkl: with timer support\n");
}
