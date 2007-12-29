#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>

#include <asm-lkl/eth.h>
#include <asm-lkl/callbacks.h>

static int sock;

static void free_rx_desc(void *_rxd)
{
	struct lkl_eth_rx_desc *rxd=(struct lkl_eth_rx_desc*)_rxd;

	free(rxd->data);
	free(rxd);
}

static void* rx_thread(void *netdev)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	char data[1518];
	int len;

	while (1) {
		len=recv(sock, data, sizeof(data), 0);
		if (len > 0) {
			struct lkl_eth_rx_desc *rxd;

			if (!(rxd=malloc(sizeof(*rxd))))
				continue;

			if (!(rxd->data=malloc(len))) {
				free(rxd);
				continue;
			}

			memcpy(rxd->data, data, len);
			rxd->len=len;
			rxd->netdev=netdev;
			rxd->free=free_rx_desc;

			linux_trigger_irq_with_data(ETH_IRQ, rxd);
		}
	}
}

int lkl_eth_rx_init(void *netdev, int ifindex)
{
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_ifindex = ifindex,
		.sll_protocol = htons(ETH_P_ALL)
	};
	pthread_t rxt;


	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

        if (sock < 0) {
                printf("failed to create socket: %s\n", strerror(errno));
                return -1;
        }

        if (bind(sock, (const struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
                printf("failed to bind socket: %s\n", strerror(errno));
		close(sock);
                return -1;
        }

	pthread_create(&rxt, NULL, rx_thread, netdev);

	return 0;
}


int lkl_eth_xmit(int ifindex, const char *data, int len)
{
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_halen = 6,
		.sll_ifindex = ifindex,
	};
	int err;

	memcpy(saddr.sll_addr, data, saddr.sll_halen);
	err=sendto(sock, data, len, 0, (const struct sockaddr*)&saddr, sizeof(saddr));
	if (err >= 0) 
		return 0;
	printf("xmit err=%s\n", strerror(errno));
	return err;
}




