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
#include <poll.h>
#include <net/if.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/callbacks.h>

struct handle {
	pthread_t thread;
	int ifindex, sock, connection;
	int pod[2]; /* pipe of death */
	void *netdev;
};

struct tun_packet {
	int len;
	unsigned char *data;
};

static inline void *netdev_priv(void *dev)
{
	return (char *)dev + ALIGN(sizeof(*dev), 32);
}

static void* rx_thread(void *handle)
{
	struct handle *h=(struct handle*)handle;
	struct lkl_eth_desc *led=NULL;
	struct pollfd pfd[] = {
		{ .fd = h->sock, .events = POLLIN, .revents = 0 },
		{ .fd = h->pod[0], .events = POLLIN, .revents = 0 }
	};

	lkl_printf("lkl eth: starting rx thread\n");

	while (1) {
		int len;

		poll(pfd, 2, -1);

		if (pfd[1].revents)
			break;

		if (!led) {
			led = lkl_eth_tun_get_rx_desc(h->netdev);
			if (!led) {
				lkl_printf("lkl eth: failed to get descriptor\n");
				/* drop the packet */
				recv(h->sock, NULL, 0, 0);
				continue;
			}
		}

		len=recv(h->sock, &led->len, sizeof(len), 0);
		if (len==0) {
			close(h->sock);
			close(h->connection);
			//lkl_eth_tun_native_cleanup(h);
			break;//where done folks
		}
		len=recv(h->sock, led->data, led->len, 0);
		if (len==0) {
			close(h->sock);
			close(h->connection);
			//lkl_eth_tun_native_cleanup(h);
			break;//where done folks
		}

		if (len > 0) {
			led->len=len; 
			lkl_trigger_irq_with_data(ETH_TUN_IRQ, led);
			led=NULL;
		} else 
			lkl_printf("lkl eth: failed to receive: %s\n", strerror(errno));

	}
	
	lkl_printf("lkl eth: stopping rx thread\n");

	return NULL;
}

static void* lkl_eth_tun_native_init_hub(void* netdev, const char *native_dev, struct tun_device* tun_device)
{
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = tun_device->address,
		.sin_port = htons(tun_device->port)
	};
	struct handle *h = malloc(sizeof(*h));

	if (!h) {
		lkl_printf("lkl eth: failed to allocate memory\n");
		goto out;
	}
	h->netdev = netdev;

	h->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (h->sock < 0) {
		lkl_printf("lkl eth: failed to create socket: %s\n", strerror(errno));
		goto out_free_h;
	}

	if (connect(h->sock, (struct sockaddr*) &saddr, sizeof(saddr)) < 0){
		lkl_printf("lkl eth: failed to connect to hub: %s\n", strerror(errno));
		goto out_close_sock;
	}

	return h;

out_close_sock:
	close(h->sock);
out_free_h:
	free(h);
out:
	return NULL;
}

static void* lkl_eth_tun_native_init_direct(void* netdev, const char *native_dev, struct tun_device* tun_device)
{
	struct handle *h = malloc(sizeof(*h));
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_port = htons(tun_device->port),
	};
	struct sockaddr_in other_end;
	int oe_len=1;

	if (!h) {
		lkl_printf("lkl eth: failed to allocate memory\n");
		goto out;
	}

	h->netdev = netdev;

	if (tun_device->type==TUN_DIRECT_DTE) {
		h->sock = socket(AF_INET, SOCK_STREAM, 0);
		if (h->sock < 0) {
			lkl_printf("lkl eth: failed to create socket: %s\n", strerror(errno));
			goto out_free_h;
		}
		saddr.sin_addr.s_addr = tun_device->address;
		
		if (connect(h->sock, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
			lkl_printf("lkl eth: failed to connect to hub: %s\n", strerror(errno));
			goto out_close_connection;
		}
	} else {
		saddr.sin_addr.s_addr = INADDR_ANY;
		h->connection = socket(AF_INET, SOCK_STREAM, 0);
		if (h->connection < 0) {
			lkl_printf("lkl eth: failed to create socket: %s\n", strerror(errno));
			goto out_free_h;
		}


		if (setsockopt(h->connection,SOL_SOCKET,SO_REUSEADDR,&oe_len,sizeof(int)) == -1) { 
			lkl_printf("lkl eth: failed to reuse addr : %s\n", strerror(errno));
			goto out_free_h;
		}

		if (bind(h->connection, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
			lkl_printf("lkl eth: failed to bind socket: %s\n", strerror(errno));
			goto out_close_connection;
		}

		if (listen(h->connection, 1) < 0) {
			lkl_printf("lkl eth: failed to listen on socket: %s\n", strerror(errno));
			goto out_close_connection;
		}

		h->sock = accept(h->connection, (struct sockaddr*) &other_end, &oe_len);
		if (h->sock < 0) {
			lkl_printf("lkl eth: %s\n", strerror(errno));
			goto out_close_connection;
		}
	}

	return h;

out_close_connection:
	close(h->connection);
out_free_h:
	free(h);
out:
	return NULL;

}

void* lkl_eth_tun_native_init(void* netdev, const char *native_dev, struct tun_device* tun_device)
{

	lkl_printf("lkl eth tun native init\n");
	struct handle *h;
	
	switch (tun_device->type) {
	case TUN_HUB:
		h = lkl_eth_tun_native_init_hub(netdev, native_dev, tun_device);
		break;
	case TUN_DIRECT_DTE:
		h = lkl_eth_tun_native_init_direct(netdev, native_dev, tun_device);
		break;
	case TUN_DIRECT_DCE:
		h = lkl_eth_tun_native_init_direct(netdev, native_dev, tun_device);
		break;
	default:
		lkl_printf("lkl eth: unknown device type\n");
		goto out;
	}

	if (!h) {
		lkl_printf("lkl eth: failed to allocate memory\n");
		goto out;
	}
	
	if (pipe(h->pod) < 0) {
                lkl_printf("lkl eth: pipe() failed: %s\n", strerror(errno));
		goto out_close_sock;
	}


	if (pthread_create(&h->thread, NULL, rx_thread, h) < 0) {
		lkl_printf("lkl eth: failed to create thread\n");
		goto out_close_pod;
	}	

	return h;

out_close_pod:
	close(h->pod[0]);
	close(h->pod[1]);
out_close_sock:
	close(h->sock);
out_free_h:
	free(h);
out:
	return NULL;
}

void lkl_eth_tun_native_cleanup(void *handle)
{
	struct handle *h=(struct handle*)handle;
	char c;

	write(h->pod[1], &c, 1);

	pthread_join(h->thread, NULL);

	close(h->connection);
	close(h->sock);
	close(h->pod[0]); close(h->pod[1]);

	free(h);
}

int lkl_eth_tun_native_xmit(void *handle, const char *data, int len)
{
	int err;
	struct handle *h=(struct handle*)handle;

	err = send(h->sock, &len, sizeof(len), 0);
	if (err < 0)
		goto error;
	err = send(h->sock, data, len, 0);
	if (err < 0)
		goto error;

	return 0;
		
error:
	lkl_printf("lkl_eth: failed to send %s\n", strerror(errno));
	return err;
}
