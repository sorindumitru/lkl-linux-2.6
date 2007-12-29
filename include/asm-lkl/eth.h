#ifndef _LKL_ETH_H
#define _LKL_ETH_H

int lkl_add_eth(char *mac, int native_ifindex);
int lkl_eth_xmit(int ifindex, const char *data, int len);
int lkl_eth_rx_init(void *netdev, int native_ifindex);

struct lkl_eth_rx_desc {
	void *netdev;
	char *data;
	int len;
	void (*free)(void*);
};

#endif
