#include <pthread.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include <asm-lkl/callbacks.h>

struct thread_info {
        pthread_t th;
        pthread_mutex_t sched_mutex;
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

        pthread_mutex_init(&ti->sched_mutex, NULL);
        pthread_mutex_lock(&ti->sched_mutex);
	ti->dead=0;

	return ti;
}

static void context_switch(void *prev, void *next)
{
        struct thread_info *_prev=(struct thread_info*)prev;
        struct thread_info *_next=(struct thread_info*)next;
        
        pthread_mutex_unlock(&_next->sched_mutex);
        pthread_mutex_lock(&_prev->sched_mutex);
	if (_prev->dead) {
		free(_prev);
		pthread_exit(NULL);
	}
}

static pthread_mutex_t helper_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* helper(void *arg)
{
        struct helper_arg *ha=(struct helper_arg*)arg;
        int (*fn)(void*)=ha->fn;
        void *farg=ha->arg;
        struct thread_info *ti=ha->ti;

        pthread_mutex_unlock(&helper_mutex);
        pthread_mutex_lock(&ti->sched_mutex);
        return (void*)fn(farg);
}

static void free_thread(void *arg)
{
        struct thread_info *ti=(struct thread_info*)arg;

	ti->dead=1;
        pthread_mutex_unlock(&ti->sched_mutex);
}

static int new_thread(int (*fn)(void*), void *arg, void *ti)
{
        struct helper_arg ha = {
                .fn = fn,
                .arg = arg,
                .ti = (struct thread_info*)ti
        };
        int ret;

        ret=pthread_create(&ha.ti->th, NULL, helper, &ha);
        pthread_mutex_lock(&helper_mutex);
        return ret;
}

typedef struct {
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
} pthread_sem_t;

static pthread_sem_t idle_sem = {
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.count = 0,
	.cond = PTHREAD_COND_INITIALIZER
};

static void exit_idle(void)
{
	pthread_mutex_lock(&idle_sem.lock);
	idle_sem.count++;
	if (idle_sem.count > 0)
		pthread_cond_signal(&idle_sem.cond);
	pthread_mutex_unlock(&idle_sem.lock);
}

static void enter_idle(int halted)
{
	sigset_t sigmask;

	if (halted)
		return;

	/*
	 * Avoid deadlocks by blocking signals for idle thread. A few notes:
	 *
	 * - POSIX says it will send the signal to one of the threads which has
	 * signal unblocked
	 * - we have at least 2 running threads: idle and init
	 * - only init can call this function, thus only init can block the signal
	 *
	 */
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGALRM);

	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
	pthread_mutex_lock(&idle_sem.lock);
	while (idle_sem.count <= 0)
		pthread_cond_wait(&idle_sem.cond, &idle_sem.lock);
	idle_sem.count--;
	pthread_mutex_unlock(&idle_sem.lock);
	pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
}

static unsigned long long time_ns(void)
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return tv.tv_sec*1000000000ULL+tv.tv_usec*1000ULL;
}

static void sigalrm(int sig)
{
        linux_trigger_irq(TIMER_IRQ);
}

static void timer(unsigned long delta)
{
        unsigned long long delta_us=delta/1000;
        struct timeval tv = {
                .tv_sec = delta_us/1000000,
                .tv_usec = delta_us%1000000
        };
        struct itimerval itval = {
                .it_interval = {0, },
                .it_value = tv
        };
        
        setitimer(ITIMER_REAL, &itval, NULL);
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

static pthread_mutex_t syscall_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t syscall_mutex_wait = PTHREAD_MUTEX_INITIALIZER;

static void* syscall_prepare(void)
{
	pthread_mutex_lock(&syscall_mutex);
	return NULL;
}

static void syscall_wait(void *arg)
{
	pthread_mutex_lock(&syscall_mutex_wait);
	pthread_mutex_unlock(&syscall_mutex);
}

static void syscall_done(void *arg)
{
	pthread_mutex_unlock(&syscall_mutex_wait);
}

static void print(const char *str, int len)
{
	write(1, str, len);
}

static int (*app_init)(void);

static pthread_mutex_t init_mutex=PTHREAD_MUTEX_INITIALIZER;

static int init(void)
{
	int ret=app_init();
	pthread_mutex_unlock(&init_mutex);
	return ret;
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
	.time = time_ns,
	.timer = timer,
	.init = init,
	.print = print,
	.syscall_prepare = syscall_prepare,
	.syscall_wait = syscall_wait,
	.syscall_done = syscall_done
};


static void* init_thread(void *arg)
{
	linux_start_kernel(&nops, "");
	return NULL;
}

pthread_t init_thread_handle;

void lkl_env_init(int (*_init)())
{
	app_init=_init;

        pthread_mutex_lock(&helper_mutex);
        signal(SIGALRM, sigalrm);
	pthread_mutex_lock(&syscall_mutex_wait);
	pthread_mutex_lock(&init_mutex);
        pthread_create(&init_thread_handle, NULL, init_thread, NULL);
	pthread_mutex_lock(&init_mutex);
}

