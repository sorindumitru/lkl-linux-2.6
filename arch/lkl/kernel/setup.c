#include <linux/types.h>
#include <linux/console.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/fs.h>
#include <linux/mqueue.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <linux/bootmem.h>
#include <linux/swap.h>


#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>

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

struct thread_info *_current_thread_info=&init_thread_union.thread_info;

void free_initmem(void)
{
}

extern void lkl_console_write(const char *, unsigned);
static void __lkl_console_write(struct console *con, const char *str, unsigned len)
{
        lkl_console_write(str, len);
}

static struct console lkl_console = {
	.name	= "lklcon",
	.write	= __lkl_console_write,	
	.flags	= CON_PRINTBUFFER,
	.index	= -1,
};

extern void get_cmd_line(char **);

extern long (*_panic_blink)(long);

unsigned long phys_mem,  phys_mem_size;
extern void _mem_init(unsigned long*, unsigned long*);
unsigned long empty_zero_page;


void __init mem_init_0(void)
{
	int bootmap_size;


#if 0
	memory_end = _ramend; /* by now the stack is part of the init task */

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;
#endif

        _mem_init(&phys_mem, &phys_mem_size);
        BUG_ON(!phys_mem);
        BUG_ON(phys_mem != PAGE_ALIGN(PAGE_OFFSET));
        BUG_ON(phys_mem_size % PAGE_SIZE != 0);



	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	bootmap_size = init_bootmem(virt_to_pfn(PAGE_OFFSET), virt_to_pfn(PAGE_OFFSET+phys_mem_size));

	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(phys_mem, phys_mem_size);
	reserve_bootmem(phys_mem, bootmap_size);

	/*
	 * Get kmalloc into gear.
	 */
#if 0
	empty_bad_page_table = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	empty_bad_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
#endif
	empty_zero_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	{
		unsigned long zones_size[MAX_NR_ZONES] = {0, };

		zones_size[ZONE_DMA] = 0 >> PAGE_SHIFT;
		zones_size[ZONE_NORMAL] = (phys_mem_size) >> PAGE_SHIFT;
		free_area_init(zones_size);
	}

}

asmlinkage void schedule_tail(struct task_struct *prev);
extern void _switch_to(void *prev, void *next);
void switch_to(struct task_struct *prev, struct task_struct *next, struct task_struct *last)
{
        _current_thread_info=next->thread_info;
        //kind of a hack because technically we don't run from the next process context
        if (!next->thread.sched_tail) {
                next->thread.sched_tail=1;
                schedule_tail(prev);
        }
        _switch_to(prev->thread_info+1, next->thread_info+1);
}

extern int _copy_thread(int (*f)(void*), void*, void*);

int copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	unsigned long unused,
	struct task_struct * p, struct pt_regs * regs)
{
        int (*f)(void*)=(int (*)(void*))esp;
        void *arg=(void*)unused;

        p->thread.sched_tail=0;
        return _copy_thread(f, arg, p->thread_info+1);
}


int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	return do_fork(flags | CLONE_VM | CLONE_UNTRACED, (unsigned long)fn, NULL, (unsigned long)arg, NULL, NULL);
}


extern int private_thread_info_size(void);
extern void private_thread_info_init(void *);

struct thread_info* alloc_thread_info(struct task_struct *task)
{
        struct thread_info *ti=kmalloc(sizeof(struct thread_info)+private_thread_info_size(), GFP_KERNEL);

        if (!ti)
                return NULL;
        private_thread_info_init(ti+1);
        return ti;
}

extern void destroy_thread(void *);
void free_thread_info(struct thread_info *ti)
{
        destroy_thread(ti+1);
        kfree(ti);
}

void show_stack(struct task_struct *task, unsigned long *esp)
{

}

void* current_private_thread_info(void)
{
        return (void*)(current_thread_info()+1);
}


/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */
unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	__do_IRQ(irq);
	irq_exit();
	set_irq_regs(old_regs);
	return 1;
}




unsigned long long sched_clock(void)
{
        /* no locking but a rare wrong value is not a big deal */
        return (jiffies_64 - INITIAL_JIFFIES) * (1000000000 / HZ);
}

void exit_thread(void)
{
}

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


void __init trap_init(void)
{
}

void init_IRQ(void)
{
}

void __init time_init(void)
{
}

void __init mem_init(void)
{
	unsigned long tmp;
	int codek = 0, datak = 0, initk = 0;
	extern char _etext, _stext, _sdata, _ebss, __init_begin, __init_end;

	max_mapnr = num_physpages = (((unsigned long) high_memory) - PAGE_OFFSET) >> PAGE_SHIFT;

	/* this will put all memory onto the freelists */
	totalram_pages = free_all_bootmem();


	codek = (&_etext - &_stext) >> 10;
	datak = 0;//(&_ebss - &_sdata) >> 10;
	initk = (&__init_begin - &__init_end) >> 10;
	tmp = nr_free_pages() << PAGE_SHIFT;
	printk(KERN_INFO "Memory available: %luk/%luk RAM, (%dk kernel code, %dk data)\n",
	       tmp >> 10,
	       phys_mem_size >> 10,
	       codek,
	       datak
	       );

}

void __devinit calibrate_delay(void)
{
}

struct seq_operations cpuinfo_op;

extern void cpu_wait_events(void);

void cpu_idle(void)
{
	while (1) {
		while (!need_resched()) 
                        cpu_wait_events();
		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}


/*
 * The internal representation of our device.
 */
struct sbull_dev {
        int size, fd;                       /* Device size in sectors */
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
} sbull_dev;


extern void _sbull_transfer(int fd, unsigned long sector, unsigned long nsect, char *buffer, int dir);

static void sbull_request(request_queue_t *q)
{
	struct request *req;

	while ((req = elv_next_request(q)) != NULL) {
		struct sbull_dev *dev = req->rq_disk->private_data;
		if (! blk_fs_request(req)) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			end_request(req, 0);
			continue;
		}
		_sbull_transfer(dev->fd, req->sector, req->current_nr_sectors,
				req->buffer, rq_data_dir(req));
		end_request(req, 1);
	}
}


extern int _sbull_open(void);
static int sbull_open(struct inode *inode, struct file *filp)
{
	struct sbull_dev *dev = inode->i_bdev->bd_disk->private_data;

        dev->fd=_sbull_open();
	filp->private_data = dev;
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbull_ops = {
	.owner           = THIS_MODULE,
	.open 	         = sbull_open,
};


extern unsigned long _sbull_sectors(void);

/*
 * Set up our internal device.
 */
void setup_device(struct sbull_dev *dev, int which)
{
        unsigned long nsectors=_sbull_sectors();

	memset (dev, 0, sizeof (struct sbull_dev));
	dev->size = nsectors*512;
	spin_lock_init(&dev->lock);
	
        dev->queue = blk_init_queue(sbull_request, &dev->lock);
        if (dev->queue == NULL)
                return;

	blk_queue_hardsect_size(dev->queue, 512);
	dev->queue->queuedata = dev;
	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(1);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
                return;
	}
	dev->gd->major = 42;
	dev->gd->first_minor = which*1;
	dev->gd->fops = &sbull_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "sbull%c", which + 'a');
	set_capacity(dev->gd, nsectors);
	add_disk(dev->gd);
	return;
}



int __init sbull_init(void)
{
	int sbull_major;
	/*
	 * Get registered.
	 */
        printk("sbull\n");
	sbull_major = register_blkdev(42, "sbull");
	if (sbull_major < 0) {
		printk(KERN_WARNING "sbull: unable to get major number: %d\n", sbull_major);
		return -EBUSY;
	}

        setup_device(&sbull_dev, 0);
    
	return 0;
}


void __init setup_arch(char **cl)
{
        get_cmd_line(cl);
        panic_blink=_panic_blink;
        private_thread_info_init((struct thread_info*)(&init_thread_union)+1);
	register_console(&lkl_console);

        mem_init_0();
}

late_initcall(sbull_init);
