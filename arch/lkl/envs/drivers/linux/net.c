#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "lkl/net.h"

static int sock;

pthread_t rxt;

static void* rx_thread(void *arg)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	char data[1600];
	int len;
	
	while (1) {
		len=recv(sock, &data[2], sizeof(data), 0);
		if (len > 0) {
			*((short*)data)=len;
			linux_trigger_irq_with_data(LKL_NET_IRQ, data);
		}
	}
}

int lkl_net_init(void)
{
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_ifindex = 11,
		.sll_protocol = htons(ETH_P_ALL)
	};
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

	pthread_create(&rxt, NULL, rx_thread, NULL);

	return 0;
}

void lkl_net_fini(void)
{
	pthread_cancel(rxt);
}


int lkl_net_xmit(const char *data, int len)
{
	struct sockaddr_ll saddr = {
		.sll_family = AF_PACKET,
		.sll_halen = 6,
		.sll_ifindex = 11,
	};
	int err;

	memcpy(saddr.sll_addr, data, saddr.sll_halen);
	err=sendto(sock, data, len, 0, (const struct sockaddr*)&saddr, sizeof(saddr));
	if (err >= 0) 
		return 0;
	printf("xmit err=%s\n", strerror(errno));
	return err;
}




