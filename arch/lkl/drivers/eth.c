#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>

#include <asm/eth.h>

static struct net_device *netdev;

static int start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	printk("tx: %d\n", skb->len);
	return lkl_eth_xmit(skb->data, skb->len);
}

static irqreturn_t lkl_net_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	const char *data=regs->irq_data;
	int len=*((unsigned short*)data);
	struct sk_buff *skb=dev_alloc_skb(len);

	data+=2;
	memcpy(skb_put(skb, len), data, len);
	skb->dev=netdev;
	skb->protocol=eth_type_trans(skb, netdev);

	printk("rx: len=%d data=%p\n", len, data);

	netif_rx(skb);
	
	return IRQ_HANDLED;
}

int nopen(struct net_device *netdev)
{
	printk("%s\n", __FUNCTION__);
	return 0;
}

static int failed_init;

int lkl_add_eth(char *mac)
{
	int err;

	if (failed_init)
		return -EBUSY;

	if (!(netdev=alloc_etherdev(0))) {
		printk("lkl eth: unable to allocate device\n");
		return -ENOMEM;
	}

	memcpy(netdev->dev_addr, mac, 6);

	netdev->hard_start_xmit=start_xmit;
	netdev->open=nopen;

	if ((err=register_netdev(netdev))) {
		printk("lkl eth: unable to register device: %d\n", err);
		return err;
	}

	printk("lkl eth: registered device %s / %d\n", netdev->name, netdev->ifindex);

	return netdev->ifindex;
}

int eth_init(void)
{
	int err;

	if ((err=request_irq(ETH_IRQ, lkl_net_irq, 0, "lkl_net", NULL))) {
		printk(KERN_ERR "lkl eth: unable to register irq %d: %d\n", ETH_IRQ, err);
		failed_init=1;
		return err;
	}
	return 0;
}

late_initcall(eth_init);
