#include <linux/sched.h>
#include <asm/ptrace.h>

void show_regs(struct pt_regs *regs)
{
  printk(KERN_ALERT "dummy lkl show_regs() impl\n");
}

