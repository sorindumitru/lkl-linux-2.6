#include <windows.h>
#include <assert.h>
#include <unistd.h>

#include <asm-lkl/callbacks.h>
#include <asm-lkl/env.h>

static void* sem_alloc(int count)
{
        return CreateSemaphore(NULL, count, 100, NULL);
}

static void sem_up(void *sem)
{
	ReleaseSemaphore(sem, 1, NULL);
}

static void sem_down(void *sem)
{
        WaitForSingleObject(sem, INFINITE);
}

static void sem_free(void *sem)
{
	CloseHandle(sem);
}

static void* thread_create(void (*fn)(void*), void *arg)
{
        return CreateThread(NULL, 0, (DWORD WINAPI (*)(LPVOID arg))fn, arg, 0, NULL);
}

static void thread_exit(void *arg)
{
	ExitThread(0);
}


/*
 * With 64 bits, we can cover about 583 years at a nanosecond resolution.
 * Windows counts time from 1601 so we do have about 100 years before we
 * overflow.
 */
static unsigned long long time(void)
{
	SYSTEMTIME st;
	FILETIME ft;
	LARGE_INTEGER li;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	
        return li.QuadPart*100;
}

static HANDLE timer;

static void set_timer(unsigned long delta)
{
	LARGE_INTEGER li = {
		.QuadPart = -((long)(delta/100)),
	};
        
	SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
}

/* FIXME: terminate thread at shutdown */
static DWORD WINAPI timer_thread(LPVOID arg)
{
	while (1) {
		WaitForSingleObject(timer, INFINITE);
		linux_trigger_irq(TIMER_IRQ);
	}
}

static long panic_blink(long time)
{
	assert(0);
	return 0;
}

static void print(const char *str, int len)
{
	write(1, str, len);
}

static HANDLE init_sem;

static int (*app_init)(void);

static int init(void)
{
	int ret=app_init();
	ReleaseSemaphore(init_sem, 1, NULL);
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
	.time = time,
	.timer = set_timer,
	.init = init,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free
};


static DWORD WINAPI init_thread(LPVOID arg)
{
	linux_start_kernel(&nops, "");
	return 0;
}

/* FIXME: check for errors */
int lkl_env_init(int (*_init)(void), unsigned long mem_size)
{
	app_init=_init;
	nops.phys_mem_size=mem_size;

	timer=CreateWaitableTimer(NULL, FALSE, NULL);
	init_sem=CreateSemaphore(NULL, 0, 100, NULL);
        CreateThread(NULL, 0, timer_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
        WaitForSingleObject(init_sem, INFINITE);

	return 0;
}

