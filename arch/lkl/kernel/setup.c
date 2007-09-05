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

int show_interrupts(struct seq_file *p, void *v)
{
        return 0;
}


void __udelay(unsigned long usecs)
{

}

void __ndelay(unsigned long nsecs)
{

}

void __init trap_init(void)
{
}

void __init time_init(void)
{
}

void __devinit calibrate_delay(void)
{
}

void show_mem(void)
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


int kernel_execve(const char *filename, char *const argv[], char *const envp[])
{
        if (strcmp(filename, "/bin/init") == 0) {
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

	return 0;
}
