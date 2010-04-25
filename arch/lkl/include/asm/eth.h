#ifndef _LKL_ETH_H
#define _LKL_ETH_H

int _lkl_add_eth(const char *native_dev, const char *mac, int rx_ring_len);
int _lkl_del_eth(int ifindex);

int lkl_eth_native_xmit(void *handle, const char *data, int len);
void* lkl_eth_native_init(void *netdev, const char *native_dev);
void lkl_eth_native_cleanup(void *handle);

struct lkl_eth_desc {
	char *data;
	int len;
};
struct lkl_eth_desc* lkl_eth_get_rx_desc(void *netdev);


#define TUN_HUB	1
#define TUN_DIRECT_DCE	2
#define TUN_DIRECT_DTE	3

/**
 * \struct lkl_tun_device 
 * @brief Interface description from user-space
 * \var type Type of interface.
 * \var port The port where the other end cand be found
 * \var address The IPv4 address of the other end
 */
struct tun_device {
	int type;
	int port;
	unsigned long address;
};

int _lkl_add_eth_tun(const char *native_dev, const char *mac, int rx_ring_len, struct tun_device *tun_device);
int _lkl_del_eth_tun(int ifindex);

int lkl_eth_tun_native_xmit(void *handle, const char *Data, int len);
void* lkl_eth_tun_native_init(void *netdev, const char *native_dev, struct tun_device* tun_device);
void lkl_eth_tun_native_cleanup(void *handle);

#ifndef __KERNEL__
#include <asm/lkl.h>
static inline int lkl_add_eth(const char *native_dev, const char *mac, int rx_ring_len)
{
	return lkl_sys_call((long)_lkl_add_eth, (long)native_dev, (long)mac,
			    rx_ring_len, 0, 0);
}
static inline int lkl_del_eth(int ifindex)
{
	return lkl_sys_call((long)_lkl_del_eth, ifindex, 0, 0, 0, 0);
}
static inline int lkl_add_eth_tun(const char *native_dev, const char *mac, int rx_ring_len, struct tun_device __user *tun_device)
{
	return lkl_sys_call((long)_lkl_add_eth_tun, (long)native_dev, (long)mac, rx_ring_len, (long)tun_device, 0);
}
static inline int lkl_del_eth_tun(int ifindex)
{
	return lkl_sys_call((long)_lkl_del_eth_tun, ifindex, 0, 0, 0, 0);
}
#endif

#endif
