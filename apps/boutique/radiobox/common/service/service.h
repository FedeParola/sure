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

typedef void (*handle_request_t)(struct unimsg_shm_desc *desc);

static size_t get_rpc_size(enum command command)
{
	ssize_t size;

	switch (command) {
	case CART_ADD_ITEM:
		size = sizeof(AddItemRequest);
		break;
	case CART_GET_CART:
		size = sizeof(GetCartRR);
		break;
	case CART_EMPTY_CART:
		size = sizeof(EmptyCartRequest);
		break;
	case RECOMMENDATION_LIST_RECOMMENDATIONS:
		size = sizeof(ListRecommendationsRR);
		break;
	case CURRENCY_GET_SUPPORTED_CURRENCIES:
		size = sizeof(GetSupportedCurrenciesResponse);
		break;
	case CURRENCY_CONVERT:
		size = sizeof(CurrencyConversionRR);
		break;
	case PRODUCTCATALOG_LIST_PRODUCTS:
		size = sizeof(ListProductsResponse);
		break;
	case PRODUCTCATALOG_GET_PRODUCT:
		size = sizeof(GetProductRR);
		break;
	case PRODUCTCATALOG_SEARCH_PRODUCTS:
		size = sizeof(SearchProductsRR);
		break;
	case SHIPPING_GET_QUOTE:
		size = sizeof(GetQuoteRR);
		break;
	case SHIPPING_SHIP_ORDER:
		size = sizeof(ShipOrderRR);
		break;
	case PAYMENT_CHARGE:
		size = sizeof(ChargeRR);
		break;
	case EMAIL_SEND_ORDER_CONFIRMATION:
		size = sizeof(SendOrderConfirmationRR);
		break;
	case CHECKOUT_PLACE_ORDER:
		size = sizeof(PlaceOrderRR);
		break;
	case AD_GET_ADS:
		size = sizeof(AdRR);
		break;
	default:
		fprintf(stderr, "Unknown gRPC command %d\n", command);
		exit(1);
	}

	return size + sizeof(struct rpc);
}

struct pending_buffer {
	struct unimsg_shm_desc desc;
	unsigned expected_sz;
};

static void move_data(struct unimsg_shm_desc *dst, struct unimsg_shm_desc *src,
		      unsigned size)
{
	memcpy(dst->addr + dst->size, src->addr, size);
	dst->size += size;
	src->addr += size;
	src->off += size;
	src->size -= size;
}

static int process_desc(struct pending_buffer *pending,
			struct unimsg_shm_desc *desc)
{
	int rc;

	if (!pending->desc.addr) {
		/* There's nothing pending */

		if (desc->size < sizeof(struct rpc)) {
			pending->desc = *desc;
			desc->size = 0;
			return 0;
		}

		struct rpc *rpc = desc->addr;
		pending->expected_sz = get_rpc_size(rpc->command);

		if (desc->size == pending->expected_sz) {
			pending->desc = *desc;
			desc->size = 0;
			return 1;

		} else if (desc->size < pending->expected_sz) {
			if (UNIMSG_BUFFER_SIZE - 68 - desc->off >=
			    pending->expected_sz) {
				/* The buffer can hold the full message */
				pending->desc = *desc;
				desc->size = 0;
			} else {
				/* The buffer can't hold the full message */
				rc = unimsg_buffer_get(&pending->desc, 1);
				if (rc) {
					fprintf(stderr, "Error getting shm "
						"buffer: %s\n", strerror(-rc));
					exit(1);
				}
				pending->desc.size = 0;
				move_data(&pending->desc, desc, desc->size);
			}

			return 0;

		} else { /* desc->size > pending->expected_sz */
			rc = unimsg_buffer_get(&pending->desc, 1);
			if (rc) {
				fprintf(stderr, "Error getting shm buffer: "
					"%s\n", strerror(-rc));
				exit(1);
			}
			pending->desc.size = 0;
			move_data(&pending->desc, desc, pending->expected_sz);

			return 1;
		}

	} else {
		/* There's something pending */
		unsigned needed = pending->expected_sz ?
				  pending->expected_sz - pending->desc.size :
				  sizeof(struct rpc) - pending->desc.size;

		unsigned to_copy = MIN(needed, desc->size);
		move_data(&pending->desc, desc, to_copy);

		if (!pending->expected_sz
		    && pending->desc.size > sizeof(struct rpc)) {
			struct rpc *rpc = pending->desc.addr;
			pending->expected_sz = get_rpc_size(rpc->command);
		}

		if (desc->size == 0)
			unimsg_buffer_put(desc, 1);

		return pending->desc.size == pending->expected_sz ? 1 : 0;
	}
}

#endif /* __SERVICE__ */