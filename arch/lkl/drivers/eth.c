#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <asm/irq_regs.h>

#include <asm/eth.h>

struct lkl_eth_data {
	int native_ifindex;
};

static int start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct lkl_eth_data *led=(struct lkl_eth_data*)netdev_priv(netdev);

	printk("tx: %d\n", skb->len);
	return lkl_eth_xmit(led->native_ifindex, skb->data, skb->len);
}

static irqreturn_t lkl_net_irq(int irq, void *dev_id)
{
	struct pt_regs *regs=get_irq_regs();
	struct lkl_eth_rx_desc *rxd=regs->irq_data;
	struct sk_buff *skb=dev_alloc_skb(rxd->len);

	memcpy(skb_put(skb, rxd->len), rxd->data, rxd->len);
	skb->dev=rxd->netdev;
	skb->protocol=eth_type_trans(skb, rxd->netdev);

	printk("rx: len=%d data=%p\n", rxd->len, rxd->data);

	rxd->free(rxd);

	netif_rx(skb);
	
	return IRQ_HANDLED;
}

int nopen(struct net_device *netdev)
{
	printk("%s\n", __FUNCTION__);
	return 0;
}

static int failed_init;

int lkl_add_eth(char *mac, int native_ifindex)
{
	struct net_device *netdev;
	struct lkl_eth_data *led;
	int err;

	if (failed_init)
		return -EBUSY;

	if (!(netdev=alloc_etherdev(sizeof(struct lkl_eth_data)))) {
		printk("lkl eth: unable to allocate device\n");
		return -ENOMEM;
	}

	led=(struct lkl_eth_data*)netdev_priv(netdev);
	led->native_ifindex=native_ifindex;

	memcpy(netdev->dev_addr, mac, 6);

	netdev->hard_start_xmit=start_xmit;
	netdev->open=nopen;

	if ((err=register_netdev(netdev))) {
		kfree(netdev);
		printk("lkl eth: unable to register device: %d\n", err);
		return err;
	}

	if (lkl_eth_rx_init(netdev, native_ifindex) < 0) {
		unregister_netdev(netdev);
		kfree(netdev);
		printk("lkl eth: failed to initialize rx on native device %d\n", native_ifindex);
		return -EBUSY;
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
