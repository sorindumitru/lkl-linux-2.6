#include <pthread.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <asm-lkl/callbacks.h>
#include <asm-lkl/env.h>


static void print(const char *str, int len)
{
	write(1, str, len);
}


typedef struct {
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
} pthread_sem_t;

static void* sem_alloc(int count)
{
	pthread_sem_t *sem=malloc(sizeof(*sem));

	if (!sem)
		return NULL;

	pthread_mutex_init(&sem->lock, NULL);
	sem->count=count;
	pthread_cond_init(&sem->cond, NULL);

	return sem;
}

static void sem_free(void *sem)
{
	free(sem);
}

static void sem_up(void *_sem)
{
	pthread_sem_t *sem=(pthread_sem_t*)_sem;

	pthread_mutex_lock(&sem->lock);
	sem->count++;
	if (sem->count > 0)
		pthread_cond_signal(&sem->cond);
	pthread_mutex_unlock(&sem->lock);
}

static void sem_down(void *_sem)
{
	pthread_sem_t *sem=(pthread_sem_t*)_sem;
	
	pthread_mutex_lock(&sem->lock);
	while (sem->count <= 0)
		pthread_cond_wait(&sem->cond, &sem->lock);
	sem->count--;
	pthread_mutex_unlock(&sem->lock);
}

static void* thread_create(void (*fn)(void*), void *arg)
{
        int ret;
	pthread_t *thread=malloc(sizeof(*thread));

	if (!thread)
		return NULL;

        ret=pthread_create(thread, NULL, (void* (*)(void*))fn, arg);
	if (ret != 0) {
		free(thread);
		return NULL;
	}

        return thread;
}

static void thread_exit(void *thread)
{
	free(thread);
	pthread_exit(NULL);
}

static unsigned long long time_ns(void)
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return tv.tv_sec*1000000000ULL+tv.tv_usec*1000ULL;
}

static int timer_pipe[2];

static void timer(unsigned long delta)
{
	write(timer_pipe[1], &delta, sizeof(delta));
}

/*
 * FIXME: destroy thread at shutdown.
 */
static void* timer_thread(void *arg)
{
	long timeout_ms=-1;
	unsigned long timeout_ns;
	struct pollfd pf = {
		.events = POLLIN,
	};
	int err;


	if (pipe(timer_pipe) < 0) {
		printf("lkl: unable to create timer pipe\n");
		return NULL;
	}
	pf.fd=timer_pipe[0];

	while (1) {
		err=poll(&pf, 1, timeout_ms);
		timeout_ms=-1;

		switch (err) {
		case 1:
			read(timer_pipe[0], &timeout_ns, sizeof(unsigned long));
			timeout_ms=timeout_ns/1000000; 
			/* 
			 * while(1){poll(,,0);} is really while(1);. Not to
			 * mention that we will generate a zillion interrupts
			 * while doing it.
			 */
			if (!timeout_ms)
				timeout_ms++;
			break;
		default:
			printf("lkl: timer error: %d %s!\n", err, strerror(errno));
			/* fall through */
		case 0:
			linux_trigger_irq(TIMER_IRQ);
			break;
		}
	}

	return NULL;
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
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
	.thread_create = thread_create,
	.thread_exit = thread_exit,
	.sem_alloc = sem_alloc,
	.sem_free = sem_free,
	.sem_up = sem_up,
	.sem_down = sem_down,
	.time = time_ns,
	.timer = timer,
	.init = init,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free
};


static void* init_thread(void *arg)
{
	linux_start_kernel(&nops, "");
	return NULL;
}

/* FIXME: check for errors */
int lkl_env_init(int (*_init)(), unsigned long mem_size)
{
	/* don't really need them */
	pthread_t a, b;

	app_init=_init;
	nops.phys_mem_size=mem_size;

        pthread_create(&b, NULL, timer_thread, NULL);
	pthread_mutex_lock(&init_mutex);
        pthread_create(&a, NULL, init_thread, NULL);
	pthread_mutex_lock(&init_mutex);

	return 0;
}

