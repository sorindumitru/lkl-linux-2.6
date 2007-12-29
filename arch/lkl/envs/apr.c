#include <apr.h>
#include <apr_atomic.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_file_io.h>
#include <apr_time.h>
#include <apr_poll.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <asm-lkl/callbacks.h>

static apr_pool_t *pool;

struct apr_thread_wrapper_t;

struct thread_info {
        apr_thread_t *thread;
        apr_thread_mutex_t *sched_mutex;
	int dead;
};

struct helper_arg {
        int (*fn)(void*);
        void *arg;
        struct thread_info *ti;
};

static void* thread_info_alloc(void)
{
        struct thread_info *ti=malloc(sizeof(*ti));

        assert(ti != NULL);

        apr_thread_mutex_create(&ti->sched_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
        apr_thread_mutex_lock(ti->sched_mutex);
        ti->dead=0;

        return ti;
}

static void context_switch(void *prev, void *next)
{
        struct thread_info *_prev=(struct thread_info*)prev;
        struct thread_info *_next=(struct thread_info*)next;

        apr_thread_mutex_unlock(_next->sched_mutex);
        apr_thread_mutex_lock(_prev->sched_mutex);
	if (_prev->dead) {
		apr_thread_t *thread=_prev->thread;
		apr_thread_mutex_destroy(_prev->sched_mutex);
		free(_prev);
		apr_thread_exit(thread, 0);
	}
}

static apr_thread_mutex_t *helper_mutex;

static void* APR_THREAD_FUNC helper(apr_thread_t *thr, void *arg)
{
        struct helper_arg *ha=(struct helper_arg*)arg;
        int (*fn)(void*)=ha->fn;
        void *farg=ha->arg;
        struct thread_info *ti=ha->ti;

        apr_thread_mutex_unlock(helper_mutex);
        apr_thread_mutex_lock(ti->sched_mutex);
        return (void*)fn(farg);
}

static void free_thread(void *arg)
{
        struct thread_info *ti=(struct thread_info*)arg;
        ti->dead=1;
        apr_thread_mutex_unlock(ti->sched_mutex);
}

static int new_thread(int (*fn)(void*), void *arg, void *ti)
{
        struct helper_arg ha = {
                .fn = fn,
                .arg = arg,
                .ti = (struct thread_info*)ti
        };
        int rc;

        rc = apr_thread_create(&ha.ti->thread, NULL, &helper, &ha, pool);
        apr_thread_mutex_lock(helper_mutex);
        return rc;
}

typedef struct {
	apr_thread_mutex_t *lock;
	int count;
	apr_thread_cond_t *cond;
} apr_sem_t;

static unsigned long long time(void)
{
	return apr_time_now()*1000;
}


/*
 * APR does not provide timers -- the reason for this ugly hack.
 */
static unsigned long long timer_exp;
static apr_file_t *events_pipe_in, *events_pipe_out;
static apr_pollset_t *pollset;

static void timer(unsigned long delta)
{
	if (delta)
		timer_exp=time()+delta;
	else
		timer_exp=0;
}

static void exit_idle(void)
{
	char c = 0;
	apr_size_t n=1;
	
	apr_file_write(events_pipe_out, &c, &n);
}

static void enter_idle(int halted)
{
	signed long long delta=timer_exp-time();
	apr_int32_t num=0;
	const apr_pollfd_t *descriptors;

	if (!timer_exp && !halted)
		apr_pollset_poll(pollset, 0, &num, &descriptors);
	else {
		if (delta > 0 && !halted) {
			apr_pollset_poll(pollset, delta/1000, &num, &descriptors);
		}
	}

	if (num > 0) {
		char c;
		apr_size_t n=1;
		apr_file_read(events_pipe_in, &c, &n);
	}

	if (timer_exp <= time()) {
		timer_exp=0;
		linux_trigger_irq(TIMER_IRQ);
	}
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
}

static void *_phys_mem;

static void mem_init(unsigned long *phys_mem, unsigned long *phys_mem_size)
{
	*phys_mem_size=16*1024*1024;
	*phys_mem=(unsigned long)malloc(*phys_mem_size);
}

static void halt(void)
{
	free(_phys_mem);
}

static apr_thread_mutex_t *syscall_mutex;
static apr_thread_mutex_t *wait_syscall_mutex;

void syscall_done(void *arg)
{
	apr_thread_mutex_unlock(wait_syscall_mutex);
}

void* syscall_prepare(void)
{
	apr_thread_mutex_lock(syscall_mutex);
	return NULL;
}

void syscall_wait(void *arg)
{
	apr_thread_mutex_lock(wait_syscall_mutex);
	apr_thread_mutex_unlock(syscall_mutex);
}

void print(const char *data, int len)
{
	write(1, data, len);
}

static struct linux_native_operations nops = {
	.panic_blink = panic_blink,
	.mem_init = mem_init,
	.halt = halt,
	.thread_info_alloc = thread_info_alloc,
	.new_thread = new_thread,
	.free_thread = free_thread,
	.context_switch = context_switch,
	.enter_idle = enter_idle,
	.exit_idle = exit_idle,
	.time = time,
	.timer = timer,
	.syscall_prepare = syscall_prepare,
	.syscall_done = syscall_done,
	.syscall_wait = syscall_wait,
	.print = print
};

static int (*app_init)(void);
static apr_thread_mutex_t *init_mutex;

static int init(void)
{
	int ret=app_init();
        apr_thread_mutex_unlock(init_mutex);
	return ret;
}

static void* APR_THREAD_FUNC init_thread(apr_thread_t *thr, void *arg)
{
	linux_start_kernel(&nops, "");
	apr_thread_exit(thr, 0);
	return NULL;
}

static apr_thread_t *init_thread_handle;

void lkl_env_init(int (*_init)(void))
{
	app_init=_init;
	nops.init=init;

	apr_pool_create(&pool, NULL);

	apr_thread_mutex_create(&helper_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
        apr_thread_mutex_lock(helper_mutex);

	apr_pollset_create(&pollset, 1, pool, 0);
	apr_file_pipe_create(&events_pipe_in, &events_pipe_out, pool);
	apr_pollfd_t apfd = {
		.p = pool,
		.desc_type = APR_POLL_FILE,
		.reqevents = APR_POLLIN,
		.desc = {
			.f = events_pipe_in
		}
	};
	apr_pollset_add(pollset, &apfd);

	apr_thread_mutex_create(&init_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
        apr_thread_mutex_lock(init_mutex);
	apr_thread_create(&init_thread_handle, NULL, init_thread, NULL, pool);
        apr_thread_mutex_lock(init_mutex);

	apr_thread_mutex_create(&syscall_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
	apr_thread_mutex_create(&wait_syscall_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
	apr_thread_mutex_lock(wait_syscall_mutex);
}

