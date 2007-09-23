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
#include <linux/reboot.h>

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

int linux_halted=0;

void kill_all_threads(void)
{
	struct task_struct *p;

	for_each_process(p) {
		BUG_ON(p->state == TASK_RUNNING);
		free_thread_info(task_thread_info(p));
	}
}

void machine_halt(void)
{
	linux_halted=1;
}

void machine_power_off(void)
{
	machine_halt();
}

void machine_restart(char * __unused)
{
	machine_halt();
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
	set_private_thread_info(&init_thread_union.thread_info, linux_nops->thread_info_alloc());

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
	/* 
	 * Don't pass through zombie, go dead directly. We do this to clean up
	 * khelper threads when hotplug is enabled. I wonder why we don't have
	 * khelper zombies on regular systems :-?
	 */
	current->exit_signal=-1;

        if (strcmp(filename, "/init") == 0) {

		if (linux_nops->enter_idle) {
			/* 
			 * Run any pending irqs, softirqs and rcus. We do this
			 * here so that we:
			 * - cleanup the khelper zombies
			 * - install the clock source (and switch to NO_HZ mode)
			 */
			synchronize_rcu();

			kernel_thread(run_main, NULL, 0);

			while (sys_wait4(-1, NULL, __WCLONE, 0) != -ECHILD)
				;

			kernel_halt();

			/* 
			 * We want to kill init without panic()ing
			 */
			init_pid_ns.child_reaper=0;
			do_exit(0);
		} else
			linux_nops->main();

		return 0;
	}
	

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

	kill_all_threads();

	linux_nops->timer(0);

	linux_nops->halt();

	return 0;
}


static int __init fs_setup(void)
{
	int fd;

	/* 
	 * To skip mounting the "real" rootfs. The current one (ramfs) is good
	 * enough for us.
	 */
	fd=sys_open("/init", O_CREAT, 0600);
	BUG_ON(fd < 0);
	sys_close(fd);
	
	return 0;
}

late_initcall(fs_setup);

