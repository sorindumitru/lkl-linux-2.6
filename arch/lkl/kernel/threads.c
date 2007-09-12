#include <linux/sched.h>
#include <asm/callbacks.h>

struct thread_info *_current_thread_info=&init_thread_union.thread_info;

asmlinkage void schedule_tail(struct task_struct *prev);

static struct task_struct *_last;

static inline void* task_private_thread_info(struct task_struct *task)
{
	return private_thread_info(task_thread_info(task));
}

void _switch_to(struct task_struct **prev, struct task_struct *next, struct task_struct *last)
{
	_last=last;
        _current_thread_info=task_thread_info(next);
        linux_nops->context_switch(task_private_thread_info(*prev), task_private_thread_info(next));
	*prev=_last;
}

struct thread_bootstrap_arg {
	int (*f)(void*);
	void *arg;
};

static int thread_bootstrap(void *arg)
{
	struct thread_bootstrap_arg *tba=(struct thread_bootstrap_arg*)arg;

	schedule_tail(_last);
	kfree(tba);

	return tba->f(tba->arg);
}

int copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	unsigned long unused,
	struct task_struct * p, struct pt_regs * regs)
{
	struct thread_bootstrap_arg *tba=kmalloc(sizeof(*tba), GFP_KERNEL);

	tba->f = (int (*)(void*))esp;
	tba->arg= (void*)unused;

        return linux_nops->new_thread(thread_bootstrap, tba, task_private_thread_info(p));
}

struct thread_info* alloc_thread_info(struct task_struct *task)
{
        struct __thread_info *ti=kmalloc(sizeof(struct __thread_info), GFP_KERNEL);

        if (!ti)
                return NULL;
        ti->private=linux_nops->thread_info_alloc();
        return (struct thread_info*)ti;
}

void free_thread_info(struct thread_info *ti)
{
        linux_nops->free_thread(private_thread_info(ti));
	kfree(ti);
}

void show_stack(struct task_struct *task, unsigned long *esp)
{
}

int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	return do_fork(flags | CLONE_VM | CLONE_UNTRACED, (unsigned long)fn, NULL, (unsigned long)arg, NULL, NULL);
}

void exit_thread(void)
{
}
