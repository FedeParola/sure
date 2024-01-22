/*
 * Some sort of Copyright
 */

#ifndef __SERVICE__
#define __SERVICE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
#include "message.h"

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 0
#endif

#if ENABLE_DEBUG
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...) (void)0
#endif

/* Service ids */
#define AD_SERVICE	       0
#define EMAIL_SERVICE	       1
#define PAYMENT_SERVICE	       2
#define CURRENCY_SERVICE       3
#define SHIPPING_SERVICE       4
#define PRODUCTCATALOG_SERVICE 5
#define CART_SERVICE	       6
#define RECOMMENDATION_SERVICE 7
#define CHECKOUT_SERVICE       8
#define FRONTEND	       9
#define NUM_SERVICES	       10

struct service_desc {
	unsigned id;
	char *name;
	uint32_t addr;
	uint16_t port;
};

#define SERVICE_ADDR(id) (0x0000000a | ((id + 1) << 24))
#define SERVICE_PORT(id) (5000 + (id + 1))
#define DEFINE_SERVICE(_id, _name) {					\
	.id = (_id),							\
	.name = _name,							\
	.addr = SERVICE_ADDR(_id),					\
	.port = SERVICE_PORT(_id)					\
}

static struct service_desc services[] = {
	DEFINE_SERVICE(AD_SERVICE, "ad"),
	DEFINE_SERVICE(EMAIL_SERVICE, "email"),
	DEFINE_SERVICE(PAYMENT_SERVICE, "payment"),
	DEFINE_SERVICE(CURRENCY_SERVICE, "currency"),
	DEFINE_SERVICE(SHIPPING_SERVICE, "shipping"),
	DEFINE_SERVICE(PRODUCTCATALOG_SERVICE, "product catalog"),
	DEFINE_SERVICE(CART_SERVICE, "cart"),
	DEFINE_SERVICE(RECOMMENDATION_SERVICE, "recommendation"),
	DEFINE_SERVICE(CHECKOUT_SERVICE, "checkout"),
	{
		.id = FRONTEND,
		.name = "frontend",
		.addr = SERVICE_ADDR(FRONTEND),
		.port = 80
	},
};

#define _ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define __unused __attribute__((unused))

typedef void (*handle_request_t)(struct unimsg_shm_desc *descs,
				 unsigned *ndescs);

static int handle_socket(struct unimsg_sock *s, handle_request_t handle_request)
{
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned nrecv = UNIMSG_MAX_DESCS_BULK;

	int rc = unimsg_recv(s, descs, &nrecv, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG("[service] Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG("[service] Received request of %d buffers\n", nrecv);

	unsigned nsend = nrecv;
	handle_request(descs, &nsend);

	rc = unimsg_send(s, descs, nsend, 0);
	if (rc) {
		unimsg_buffer_put(descs, nrecv > nsend ? nrecv : nsend);
		if (rc == -ECONNRESET) {
			unimsg_close(s);
			DEBUG("[service] Connection closed\n");
			return 1;
		} else if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			_ERR_CLOSE(s);
		}
	}

	DEBUG("[service] Sent response of %d buffers\n", nsend);

	/* Free excess buffers */
	if (nsend < nrecv)
		unimsg_buffer_put(descs + nsend, nrecv - nsend);

	return 0;
}

static void run_service(unsigned id, handle_request_t handle_request)
{
	int rc;
	struct unimsg_sock *socks[UNIMSG_MAX_NSOCKS];
	int ready[UNIMSG_MAX_NSOCKS];
	unsigned nsocks = 1;

	rc = unimsg_socket(&socks[0]);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_bind(socks[0], services[id].port);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n",
			services[id].port, strerror(-rc));
		_ERR_CLOSE(socks[0]);
	}

	rc = unimsg_listen(socks[0]);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		_ERR_CLOSE(socks[0]);
	}

	DEBUG("[service] Waiting for incoming connections...\n");

	while (1) {
		rc = unimsg_poll(socks, nsocks, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		for (unsigned i = 1; i < nsocks; i++) {
			if (ready[i]) {
				rc = handle_socket(socks[i], handle_request);
				if (rc) {
					/* The socket was closed, remove it from
					 * the list and shift all other sockets
					 */
					nsocks--;
					for (unsigned j = i; j < nsocks; j++) {
						socks[j] = socks[j + 1];
						ready[j] = ready[j + 1];
					}
				}
			}
		}

		if (ready[0]) {
			struct unimsg_sock *s;
			rc = unimsg_accept(socks[0], &s, 1);
			if (rc) {
				fprintf(stderr, "Error accepting connection: "
					"%s\n", strerror(-rc));
				_ERR_CLOSE(socks[0]);
			}

			socks[nsocks++] = s;

			DEBUG("[service] New client connected\n");
		}
	}
}

static struct unimsg_sock *socks[NUM_SERVICES];

__unused
static void do_rpc(struct unimsg_shm_desc *desc, unsigned service,
		   size_t rr_size)
{
	int rc = unimsg_send(socks[service], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[service], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(struct rpc) + rr_size) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

#endif /* __SERVICE__ */