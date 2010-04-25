#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>
#include <linux/string.h>

#include <asm/eth.h>
#include <asm/callbacks.h>

#define MAX_FRAME_LENGTH 1522 /* 1518+sizeof(int), adjusted for size of packet */

struct tun_data {
	void *native_handle;
	const char *native_dev;
	struct list_head rx_ring_ready, rx_ring_inuse;
	void *rx_ring_lock;
	struct tun_device tun_device;
};

struct rx_ring_entry {
	struct lkl_eth_desc led;
	struct net_device *netdev;
	struct sk_buff *skb;
	struct list_head list;
};

static int start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);
	int err=lkl_eth_tun_native_xmit(td->native_handle, skb->data, skb->len);

	if (err == 0)
		dev_kfree_skb(skb);

	return err;
}

static int reload_rx_ring_entry(struct net_device *netdev,
				struct rx_ring_entry *rre)
{
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);
	struct sk_buff *skb=dev_alloc_skb(MAX_FRAME_LENGTH);

	if (!skb)
		return -ENOMEM;

	rre->led.data=skb->data;
	rre->led.len=MAX_FRAME_LENGTH;
	rre->skb=skb;

	lkl_nops->sem_down(td->rx_ring_lock);
	/* remove it from the inuse list */
	list_del(&rre->list);
	/* and put it on the ready list */
	list_add(&rre->list, &td->rx_ring_ready);
	lkl_nops->sem_up(td->rx_ring_lock);
	
	return 0;
}

struct lkl_eth_desc* lkl_eth_tun_get_rx_desc(void *_netdev)
{
	struct net_device *netdev=(struct net_device*)_netdev;
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);
	struct rx_ring_entry *rre=NULL;

	lkl_nops->sem_down(td->rx_ring_lock);
	if (!list_empty(&td->rx_ring_ready)) {
		rre=list_first_entry(&td->rx_ring_ready, struct rx_ring_entry, list);
		list_del(&rre->list);
		/* don't forget to add it on the inuse list */
		list_add(&rre->list, &td->rx_ring_inuse);
	}
	lkl_nops->sem_up(td->rx_ring_lock);

	if (rre)
		return &rre->led;

	return NULL;
}

static irqreturn_t lkl_tun_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct lkl_eth_desc *led=(struct lkl_eth_desc*)regs->irq_data;
	struct rx_ring_entry *rre=container_of(led, struct rx_ring_entry, led);

	skb_put(rre->skb, rre->led.len);
	rre->skb->protocol=eth_type_trans(rre->skb, rre->netdev);

	netif_rx(rre->skb);

	reload_rx_ring_entry(rre->netdev, rre);

	return IRQ_HANDLED;
}

static int failed_init=1;

static void cleanup_netdev(struct net_device *netdev)
{
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);
	struct list_head *i, *aux;

	if (td->native_handle)
		lkl_eth_tun_native_cleanup(td->native_handle);

	list_for_each_safe(i, aux, &td->rx_ring_inuse) {
		struct rx_ring_entry *rre=list_entry(i, struct rx_ring_entry, list);
		list_del(&rre->list);
		kfree(rre);
	}

	list_for_each_safe(i, aux, &td->rx_ring_ready) {
		struct rx_ring_entry *rre=list_entry(i, struct rx_ring_entry, list);
		list_del(&rre->list);
		kfree(rre);
	}

	if (td->rx_ring_lock)
		lkl_nops->sem_free(td->rx_ring_lock);

	kfree(netdev);
}

int _lkl_del_eth_tun(int ifindex)
{
	struct net_device *netdev=dev_get_by_index(&init_net,ifindex);

	if (!netdev)
		return -1;

	dev_put(netdev);

	unregister_netdev(netdev);

	cleanup_netdev(netdev);

	return 0;
}

static int open(struct net_device* netdev)
{
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);
	
	if (!(td->native_handle=lkl_eth_tun_native_init(netdev, td->native_dev,&td->tun_device)))
		return -EUNATCH;

	return 0;
}

static int stop(struct net_device* netdev)
{
	struct tun_data *td=(struct tun_data*)netdev_priv(netdev);

	lkl_eth_tun_native_cleanup(td->native_handle);
	td->native_handle=NULL;

	lkl_purge_irq_queue(ETH_TUN_IRQ);

	return 0;
}

int _lkl_add_eth_tun(const char *native_dev, const char *mac, int rx_ring_len, struct tun_device __user* tun_device)
{
	int i;
	struct net_device *netdev;
	struct tun_data *netdata;

	if (failed_init) {
		return -EUNATCH;
	}

	if (!(netdev=alloc_etherdev(sizeof(struct tun_data)))) 
		return -ENOMEM;

	netdata=(struct tun_data*)netdev_priv(netdev);

	memset(netdata, 0, sizeof(*netdata));

	netdata->tun_device.port = tun_device->port;
	netdata->tun_device.address = tun_device->address;
	netdata->tun_device.type = tun_device->type;

	INIT_LIST_HEAD(&netdata->rx_ring_ready);
	INIT_LIST_HEAD(&netdata->rx_ring_inuse);

	if (!(netdata->rx_ring_lock=lkl_nops->sem_alloc(1))) 
		goto error;

	for(i=0; i<rx_ring_len; i++) {
		struct rx_ring_entry *rre=kmalloc(sizeof(*rre), GFP_KERNEL);
		if (!rre) 
			goto error;
		rre->netdev=netdev;
		list_add(&rre->list, &netdata->rx_ring_inuse);
		if (reload_rx_ring_entry(netdev, rre) < 0) 
			goto error;

	}

	memcpy(netdev->dev_addr, mac, 6);

	netdev->hard_start_xmit=start_xmit;
	netdev->open=open;
	netdev->stop=stop;

	netdata->native_dev=kstrdup(native_dev, GFP_KERNEL);

	if (register_netdev(netdev)) 
		goto error;

	printk(KERN_INFO "lkl eth: registered device %s / %s\n", netdev->name,
	       native_dev);

	return netdev->ifindex;
	
error:
	cleanup_netdev(netdev);
	return 0;
}

int eth_tun_init(void)
{
	int err=-ENOMEM;

	if ((err=request_irq(ETH_TUN_IRQ, lkl_tun_irq, 0, "lkl_net_tun", NULL))) {
		printk(KERN_ERR "lkl eth: unable to register irq %d: %d\n",
		       ETH_TUN_IRQ, err);
		return err;
	}

	return failed_init=0;
}

late_initcall(eth_tun_init);
