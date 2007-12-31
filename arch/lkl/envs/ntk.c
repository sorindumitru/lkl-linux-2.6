#include <ddk/ntddk.h>
#include <asm-lkl/callbacks.h>
#include <asm-lkl/env.h>

static void* sem_alloc(int count)
{
	KSEMAPHORE *sem=ExAllocatePool(PagedPool, sizeof(*sem));

	if (!sem)
		return NULL;

        KeInitializeSemaphore(sem, count, 100);	

	return sem;
}

static void sem_up(void *sem)
{
        KeReleaseSemaphore((KSEMAPHORE*)sem, 0, 1, 0);
}

static void sem_down(void *sem)
{
        KeWaitForSingleObject((KSEMAPHORE*)sem, Executive, KernelMode, FALSE, NULL);
}

static void sem_free(void *sem)
{
	ExFreePool(sem);
}

static void* thread_create(void (*fn)(void*), void *arg)
{
	void *thread;
	if (PsCreateSystemThread(&thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
				 (void DDKAPI (*)(void*))fn, arg) != STATUS_SUCCESS)
		return NULL;
	return thread;
}

static void thread_exit(void *arg)
{
	PsTerminateSystemThread(0);
}

/*
 * With 64 bits, we can cover about 583 years at a nanosecond resolution.
 * Windows counts time from 1601 so we do have about 100 years before we
 * overflow.
 */
static unsigned long long time(void)
{
	LARGE_INTEGER li;

	KeQuerySystemTime(&li);
	
        return li.QuadPart*100;
}

static KTIMER timer;

static void set_timer(unsigned long delta)
{
	if (!delta)
		KeCancelTimer(&timer);
	else
		KeSetTimer(&timer, RtlConvertLongToLargeInteger((unsigned long)(-(delta/100))), &timer_dpc);
}

/* FIXME: terminate thread at shutdown */
static void DDKAPI timer_thread(LPVOID arg)
{
	while (1) {
		KeWaitForSingleObject((KSEMAPHORE*)sem, Executive, KernelMode, FALSE, NULL);
		linux_trigger_irq(TIMER_IRQ);
	}
}

static long panic_blink(long time)
{
    DbgPrint("***Kernel panic!***");
    assert(0);
    return 0;
}

static void* mem_alloc(unsigned long size)
{
	return ExAllocatePool(NonPagedPool, size);
}

static void print(const char *str, int len)
{
	DbgPrint("%s", str);
}

static KSEMAPHORE init_sem;

static int (*app_init)(void);

static int init(void)
{
	int ret=app_init();
        KeReleaseSemaphore(init_sem, 0, 1, 0);
	return ret;
}

static struct linux_native_operations nops = {
	.panic_blink = panic_blink,
	.mem_init = mem_init,
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
	.mem_free = ExFreePool
	.mem_alloc = mem_alloc,
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

	KeInitializeTimer(&timer);
        KeInitializeSemaphore(&init_sem, 0, 100);	
        CreateThread(NULL, 0, timer_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
        KeWaitForSingleObject(&init_sem, Executive, KernelMode, FALSE, NULL);

	return 0;
}

