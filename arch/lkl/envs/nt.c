#include <windows.h>
#include <assert.h>
#include <unistd.h>

#include <asm-lkl/callbacks.h>

struct thread_info {
        HANDLE th;
        HANDLE sched_sem;
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

        ti->sched_sem=CreateSemaphore(NULL, 0, 100, NULL);
	ti->dead=0;

	return ti;
}

static void context_switch(void *prev, void *next)
{
        struct thread_info *_prev=(struct thread_info*)prev;
        struct thread_info *_next=(struct thread_info*)next;
        
        ReleaseSemaphore(_next->sched_sem, 1, NULL);
        WaitForSingleObject(_prev->sched_sem, INFINITE);
	if (_prev->dead) {
		CloseHandle(_prev->sched_sem);
		free(_prev);
		ExitThread(0);
	}
}

static HANDLE helper_sem;

static DWORD WINAPI helper(LPVOID arg)
{
        struct helper_arg *ha=(struct helper_arg*)arg;
        int (*fn)(void*)=ha->fn;
        void *farg=ha->arg;
        struct thread_info *ti=ha->ti;

        ReleaseSemaphore(helper_sem, 1, NULL);
        WaitForSingleObject(ti->sched_sem, INFINITE);
        return fn(farg);
}

static void free_thread(void *arg)
{
        struct thread_info *ti=(struct thread_info*)arg;

	ti->dead=1;
	ReleaseSemaphore(ti->sched_sem, 1, NULL);
}

static int new_thread(int (*fn)(void*), void *arg, void *ti)
{
        struct helper_arg ha = {
                .fn = fn,
                .arg = arg,
                .ti = (struct thread_info*)ti
        };

		
        ha.ti->th=CreateThread(NULL, 0, helper, &ha, 0, NULL);
	WaitForSingleObject(helper_sem, INFINITE);
        return 0;
}

static HANDLE idle_sem;

static void exit_idle(void)
{
	ReleaseSemaphore(idle_sem, 1, NULL);
}

static HANDLE timer;

static void enter_idle(int halted)
{
	HANDLE handles[]={idle_sem, timer};
	int count=sizeof(handles)/sizeof(HANDLE);
	int n;


	n=WaitForMultipleObjects(count, handles, FALSE, halted?0:INFINITE);
	
	assert(n < WAIT_OBJECT_0+count);

	n-=WAIT_OBJECT_0;

	if (n == 1) {
		linux_trigger_irq(TIMER_IRQ);
	}
	
	/* 
	 * It is OK to exit even if only the timer has expired, 
	 * as linux_trigger_irq will trigger an linux_exit_idle anyway 
	 */
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

static void set_timer(unsigned long delta)
{
	LARGE_INTEGER li = {
		.QuadPart = -((long)(delta/100)),
	};
        
	SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
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

static HANDLE syscall_sem;
static HANDLE syscall_sem_wait;

static void* syscall_prepare(void)
{
        WaitForSingleObject(&syscall_sem, INFINITE);
	return NULL;
}

static void syscall_wait(void *arg)
{
        WaitForSingleObject(syscall_sem_wait, INFINITE);
        ReleaseSemaphore(syscall_sem, 1, NULL);
}

static void syscall_done(void *arg)
{
        ReleaseSemaphore(syscall_sem_wait, 1, NULL);
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
	.mem_init = mem_init,
	.halt = halt,
	.thread_info_alloc = thread_info_alloc,
	.new_thread = new_thread,
	.free_thread = free_thread,
	.context_switch = context_switch,
	.enter_idle = enter_idle,
	.exit_idle = exit_idle,
	.time = time,
	.timer = set_timer,
	.syscall_prepare = syscall_prepare,
	.syscall_done = syscall_done,
	.syscall_wait = syscall_wait,
	.print = print,
	.init = init
};


static DWORD WINAPI init_thread(LPVOID arg)
{
	linux_start_kernel(&nops, "");
	return 0;
}

HANDLE init_thread_handle;

void lkl_env_init(int (*_init)(void))
{
	app_init=_init;
	nops.init=init;

        helper_sem=CreateSemaphore(NULL, 0, 100, NULL);
        idle_sem=CreateSemaphore(NULL, 0, 100, NULL);
	timer=CreateWaitableTimer(NULL, FALSE, NULL);
        syscall_sem_wait=CreateSemaphore(NULL, 0, 100, NULL);
        syscall_sem=CreateSemaphore(NULL, 1, 100, NULL);
	init_sem=CreateSemaphore(NULL, 0, 100, NULL);
        init_thread_handle=CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
        WaitForSingleObject(init_sem, INFINITE);
}

