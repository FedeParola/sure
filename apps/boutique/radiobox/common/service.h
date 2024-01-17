/*
 * Some sort of Copyright
 */

#ifndef __SERVICE__
#define __SERVICE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>

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
#define NUM_SERVICES	       9

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
};

#define FRONTEND_PORT 80

#define MAX_SOCKS 8
#define _ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })

__attribute__((unused))
static void run_service(unsigned id,
			void (*handle_request)(struct unimsg_sock *))
{
	int rc;
	struct unimsg_sock *socks[MAX_SOCKS];
	int ready[MAX_SOCKS];
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

	printf("Waiting for incoming connections...\n");

	while (1) {
		rc = unimsg_poll(socks, nsocks, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		for (unsigned i = 1; i < nsocks; i++) {
			if (ready[i])
				handle_request(socks[i]);
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

			printf("New client connected\n");
		}
	}
}

#endif /* __SERVICE__ */