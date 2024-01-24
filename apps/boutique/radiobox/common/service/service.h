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

#endif /* __SERVICE__ */