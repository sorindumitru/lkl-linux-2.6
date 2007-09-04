#ifndef _ASM_LKL_PTRACE_H
#define _ASM_LKL_PTRACE_H

struct pt_regs { 
	void *irq_data;
};

static inline int user_mode(struct pt_regs *regs)
{
        return 0;
}

static inline long profile_pc(struct pt_regs *regs)
{
        return 0;
}

#define ptrace_signal_deliver(regs, cookie) 

static inline void ptrace_disable(struct task_struct *child)
{ 
}

#endif
