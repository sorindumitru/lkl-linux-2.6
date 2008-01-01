#include <linux/kernel.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>

#include <asm/disk.h>

/*
 * The internal representation of our device.
 */
struct lkl_disk_dev {
	void *data;
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
};

static void complete_request(struct lkl_disk_cs *cs)
{
	struct request *req=(struct request*)cs->linux_cookie;

	end_that_request_first(req, cs->status, req->hard_nr_sectors);
	end_that_request_last(req, cs->status);

	kfree(cs);
}

static irqreturn_t lkl_disk_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();

	complete_request(regs->irq_data);

	return IRQ_HANDLED;
}

static void lkl_disk_request(request_queue_t *q)
{
	struct request *req;

	while ((req = elv_next_request(q)) != NULL) {
		struct lkl_disk_dev *dev = req->rq_disk->private_data;
		struct lkl_disk_cs *cs;

		if (!(cs=kmalloc(sizeof(*cs), GFP_KERNEL))) {
			end_request(req, 0);
			continue;
		}

		if (! blk_fs_request(req)) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			end_request(req, 0);
			continue;
		}

		blkdev_dequeue_request(req);

		cs->linux_cookie=req;
		lkl_disk_do_rw(dev->data, req->sector, req->current_nr_sectors,
			       req->buffer, rq_data_dir(req), cs);
		if (cs->sync)
			complete_request(cs);
	}
}

static int lkl_disk_open(struct inode *inode, struct file *filp)
{
	struct lkl_disk_dev *dev = inode->i_bdev->bd_disk->private_data;

	filp->private_data = dev;
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations lkl_disk_ops = {
	.owner           = THIS_MODULE,
	.open 	         = lkl_disk_open,
};


static int major, failed_init;

__kernel_dev_t lkl_disk_add_disk(void *data, const char *name, int which, int sectors)
{
	struct lkl_disk_dev *dev;

	if (failed_init)
		return 0;

        if (!(dev=kmalloc(sizeof(*dev), GFP_KERNEL)))
		return 0;

	memset (dev, 0, sizeof(*dev));

        dev->data=data;

	spin_lock_init(&dev->lock);
	
        if (!(dev->queue=blk_init_queue(lkl_disk_request, &dev->lock))) {
		kfree(dev);
		return 0;
	}

	blk_queue_hardsect_size(dev->queue, 512);
	dev->queue->queuedata = dev;

	if (!(dev->gd=alloc_disk(1))) {
		kfree(dev);
		/* FIXME: del queue? */
		return 0;
	}


	dev->gd->major = major;
	dev->gd->first_minor = which;
	dev->gd->fops = &lkl_disk_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "%s", name);
	set_capacity(dev->gd, sectors);

	add_disk(dev->gd);

	printk("lkldisk: attached %s @ dev=%d:%d\n", dev->gd->disk_name, dev->gd->major, dev->gd->first_minor);
	return new_encode_dev(MKDEV(dev->gd->major, dev->gd->first_minor));
}

static int __init disk_init(void)
{
	int err;

	if ((err=request_irq(DISK_IRQ, lkl_disk_irq, 0, "lkldisk", NULL))) {
		failed_init=1;
		printk(KERN_ERR "lkldisk: unable to register irq %d: %d\n", DISK_IRQ, err);
		return err;
	}

	major = register_blkdev(0, "fd");
	if (major < 0) {
		failed_init=1;
		printk(KERN_ERR "fd: unable to register_blkdev: %d\n", major);
		free_irq(DISK_IRQ, NULL);
		return -EBUSY;
	}

	return 0;
}

late_initcall(disk_init);
