#include <linux/sched.h>
#include <asm/callbacks.h>

struct thread_info *_current_thread_info=&init_thread_union.thread_info;

asmlinkage void schedule_tail(struct task_struct *prev);

void switch_to(struct task_struct *prev, struct task_struct *next, struct task_struct *last)
{
        _current_thread_info=task_thread_info(next);
        //kind of a hack because technically we don't run from the next process context
        if (!next->thread.sched_tail) {
                next->thread.sched_tail=1;
                schedule_tail(prev);
        }
        linux_switch_to(task_thread_info(prev)+1, task_thread_info(next)+1);
}

int copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	unsigned long unused,
	struct task_struct * p, struct pt_regs * regs)
{
        int (*f)(void*)=(int (*)(void*))esp;
        void *arg=(void*)unused;

        p->thread.sched_tail=0;
        return linux_new_thread(f, arg, task_thread_info(p)+1);
}

struct thread_info* alloc_thread_info(struct task_struct *task)
{
        struct thread_info *ti=kmalloc(sizeof(struct thread_info)+linux_thread_info_size, GFP_KERNEL);

        if (!ti)
                return NULL;
        linux_thread_info_init(ti+1);
        return ti;
}

void free_thread_info(struct thread_info *ti)
{
        linux_free_thread(ti+1);
        kfree(ti);
}

void show_stack(struct task_struct *task, unsigned long *esp)
{
}

void* current_private_thread_info(void)
{
        return (void*)(current_thread_info()+1);
}

int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	return do_fork(flags | CLONE_VM | CLONE_UNTRACED, (unsigned long)fn, NULL, (unsigned long)arg, NULL, NULL);
}

void exit_thread(void)
{
}
