#include <linux/sched.h>
#include <asm/callbacks.h>

/* 
 * Hide per thread information here, so that it does not get overwritten in
 * setup_thread_stack (in dup_task_struct)
 */
struct __thread_info {
	struct thread_info ti;
	void *sched_sem, *thread;
	int dead;
};

static int threads_counter;
static void *threads_counter_lock;

static inline void threads_counter_inc(void)
{
	linux_nops->sem_down(threads_counter_lock);
	threads_counter++;
	linux_nops->sem_up(threads_counter_lock);
}
	
static inline void threads_counter_dec(void)
{
	linux_nops->sem_down(threads_counter_lock);
	threads_counter--;
	linux_nops->sem_up(threads_counter_lock);
}

static inline int threads_counter_get(void)
{
	int counter;
	linux_nops->sem_down(threads_counter_lock);
	counter=threads_counter;
	linux_nops->sem_up(threads_counter_lock);

	return counter;
}


void threads_init(void)
{
	struct __thread_info *ti=(struct __thread_info*)&init_thread_union.thread_info;

	ti->dead=0;
	BUG_ON((ti->sched_sem=linux_nops->sem_alloc(0)) == NULL);
	BUG_ON((threads_counter_lock=linux_nops->sem_alloc(1)) == NULL);
}

void threads_cleanup(void)
{
	struct __thread_info *ti=(struct __thread_info*)&init_thread_union.thread_info;

	while (threads_counter_get())
		schedule_timeout(1);

	linux_nops->sem_free(ti->sched_sem);
}

struct thread_info* alloc_thread_info(struct task_struct *task)
{
        struct __thread_info *ti=kmalloc(sizeof(struct __thread_info), GFP_KERNEL);

        if (!ti)
                return NULL;

        ti->dead=0;
	if (!(ti->sched_sem=linux_nops->sem_alloc(0))) {
		kfree(ti);
		return NULL;
	}
        return (struct thread_info*)ti;
}

void free_thread_info(struct thread_info *_ti)
{
	struct __thread_info *ti = (struct __thread_info*)_ti;

	ti->dead=1;
        linux_nops->sem_up(ti->sched_sem);
}

struct thread_info *_current_thread_info=&init_thread_union.thread_info;
static struct task_struct *_last;

void _switch_to(struct task_struct **prev, struct task_struct *next, struct task_struct *last)
{
        struct __thread_info *_prev=(struct __thread_info*)task_thread_info(*prev);
        struct __thread_info *_next=(struct __thread_info*)task_thread_info(next);

	_last=last;
	_current_thread_info=task_thread_info(next);

        linux_nops->sem_up(_next->sched_sem);
        linux_nops->sem_down(_prev->sched_sem);
	if (_prev->dead) {
		void *thread=_prev->thread;
		linux_nops->sem_free(_prev->sched_sem);
		kfree(_prev);
		threads_counter_dec();
		linux_nops->thread_exit(thread);
	}

	*prev=_last;
}

asmlinkage void schedule_tail(struct task_struct *prev);

struct thread_bootstrap_arg {
	struct __thread_info *ti;
	int (*f)(void*);
	void *arg;
};

static void thread_bootstrap(void *_tba)
{
	struct thread_bootstrap_arg *tba=(struct thread_bootstrap_arg*)_tba;
	struct __thread_info *ti=tba->ti;
	int (*f)(void*)=tba->f;
	void *arg=tba->arg;
	
	kfree(tba);
        linux_nops->sem_down(ti->sched_sem);
	BUG_ON(ti->dead);
	schedule_tail(_last);

	f(arg);
}

int copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	unsigned long unused,
	struct task_struct * p, struct pt_regs * regs)
{
	struct thread_bootstrap_arg *tba=kmalloc(sizeof(*tba), GFP_KERNEL);
	struct __thread_info *ti=(struct __thread_info*)task_thread_info(p);

	tba->f=(int (*)(void*))esp;
	tba->arg=(void*)unused;
	tba->ti=ti;

        ti->thread=linux_nops->thread_create(thread_bootstrap, tba);

	if (ti->thread == NULL) {
		kfree(tba);
		return -EPERM;
	}

	threads_counter_inc();

	return 0;
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
