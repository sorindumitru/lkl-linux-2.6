#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/fs.h>
#include <linux/mqueue.h>
#include <linux/seq_file.h>
#include <linux/start_kernel.h>
#include <linux/syscalls.h>

#include <asm/callbacks.h>

struct linux_native_operations *linux_nops;

static struct fs_struct init_fs = INIT_FS;
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS(init_signals);
static struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);
struct mm_struct init_mm = INIT_MM(init_mm);

EXPORT_SYMBOL(init_mm);

union thread_union init_thread_union = { INIT_THREAD_INFO(init_task) };

/*
 * Initial task structure.
 *
 * All other task structs will be allocated on slabs in fork.c
 */
struct task_struct init_task = INIT_TASK(init_task);

EXPORT_SYMBOL(init_task);


long arch_ptrace(struct task_struct *child, long request, long addr, long data)
{
        return -EINVAL;
}

void (*pm_power_off)(void)=NULL;


void machine_restart(char * __unused)
{
}

void machine_halt(void)
{
}

void machine_power_off(void)
{
}

void flush_thread(void)
{
}

struct seq_operations cpuinfo_op;

extern void mem_init_0(void);

static char cmd_line[128];

void __init setup_arch(char **cl)
{
        *cl=cmd_line;
        panic_blink=linux_nops->panic_blink;
        linux_nops->thread_info_init((struct thread_info*)(&init_thread_union)+1);

        mem_init_0();
}

static int run_main(void *arg)
{
	linux_nops->main();
	do_exit(0);
	return 0;
}

int kernel_execve(const char *filename, char *const argv[], char *const envp[])
{
        if (strcmp(filename, "/sbin/init") == 0) {
		/* 
		 * Let some time pass so that any pending RCU and softirqs will
		 * run. We need this to:
		 * - cleanup the khelper zombies
		 * - install the clock source (and switch to NO_HZ mode);
		 *
		 * We want this to happen before the application starts. This
		 * quirks are needed because we dont run IRQs
		 * asynchronously. Maybe better abstractions are needed to make
		 * this more obvious.
		 */
		if (linux_nops->new_sem)
			schedule_timeout_uninterruptible(100); 

		kernel_thread(run_main, NULL, 0);

		while (sys_wait4(-1, NULL, __WCLONE, 0) != -ECHILD)
			;

		return 0;
	}

	/* 
	 * Don't pass through zombie, go dead directly. We do this to clean up
	 * khelper threads when hotplug is enabled. I wonder why we don't have
	 * khelper zombies on regular systems :-?
	 */
	current->exit_signal=-1;

	return -1;
}

int linux_start_kernel(struct linux_native_operations *nops, const char *fmt, ...)
{
	va_list ap;

	linux_nops=nops;

	va_start(ap, fmt);
	vsnprintf(cmd_line, sizeof(cmd_line), fmt, ap);
	va_end(ap);

	start_kernel();

	return 0;
}
