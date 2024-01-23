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
#include "../libaco/aco.h"

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 1
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
#define MAX_COROUTINES 16

struct coroutine_args {
	/* Data of upstreaming request */
	struct unimsg_sock *up_sock;
	struct unimsg_shm_desc up_descs[UNIMSG_MAX_DESCS_BULK];
	unsigned up_ndescs;
	/* Data of downtream request */
	struct unimsg_shm_desc *down_desc;
};

struct coroutine {
	aco_t *handle;
	aco_share_stack_t *stack;
	struct coroutine_args args;
};

typedef void (*handle_request_t)(struct unimsg_shm_desc *descs,
				 unsigned *ndescs);

static struct coroutine coroutines[MAX_COROUTINES];
static struct unimsg_sock *downstream_socks[NUM_SERVICES];
static handle_request_t request_handler;
static aco_t *main_co;

static void coroutine_fn()
{
	struct coroutine_args *args = aco_get_arg();
	unsigned nsend = args->up_ndescs;

	request_handler(args->up_descs, &nsend);

	int rc = unimsg_send(args->up_sock, args->up_descs, nsend, 0);
	if (rc) {
		unimsg_buffer_put(args->up_descs, args->up_ndescs > nsend ?
				  args->up_ndescs : nsend);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			_ERR_CLOSE(args->up_sock);
		}
	}

	DEBUG("[service] Sent response of %d buffers\n", nsend);

	/* Free excess buffers */
	if (nsend < args->up_ndescs) {
		unimsg_buffer_put(args->up_descs + nsend,
				  args->up_ndescs - nsend);
	}

	aco_exit();
}

static void handle_downstream(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned ndescs = 1;

	int rc = unimsg_recv(s, &desc, &ndescs, 0);
	if (rc) {
		fprintf(stderr, "Error receiving from downstream: %s\n",
			strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG("[service] Received downstream response\n");

	/* Identify the coroutine */
	struct rpc *rpc = desc.addr;
	(void)rpc->id;
	unsigned co_id = 0;

	/* Copy args */
	*coroutines[co_id].args.down_desc = desc;

	/* Resume coroutine */
	DEBUG("[service] Resuming coroutine\n");
	aco_resume(coroutines[co_id].handle);

	/* If coroutine ended, destroy it */
	if (coroutines[co_id].handle->is_end) {
		DEBUG("[service] Destroying coroutine\n");
		aco_destroy(coroutines[co_id].handle);
	}
}

static int handle_upstream(struct unimsg_sock *s)
{
	struct coroutine_args *args = &coroutines[0].args;
	args->up_sock = s;
	args->up_ndescs = UNIMSG_MAX_DESCS_BULK;
	int rc = unimsg_recv(s, args->up_descs, &args->up_ndescs, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG("[service] Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving from upstream: %s\n",
			strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG("[service] Received request of %d buffers\n", args->up_ndescs);

	coroutines[0].handle = aco_create(main_co, coroutines[0].stack, 0,
					  coroutine_fn, &coroutines[0].args);

	aco_resume(coroutines[0].handle);

	return 0;
}

static void run_service(unsigned id, handle_request_t handler,
			int *dependencies, unsigned ndependencies)
{
	int rc;
	struct unimsg_sock *socks[UNIMSG_MAX_NSOCKS];
	int ready[UNIMSG_MAX_NSOCKS];
	unsigned nsocks = 1;

	/* Initialize coroutines */
	aco_thread_init(NULL);
	main_co = aco_create(NULL, NULL, 0, NULL, NULL);
	coroutines[0].stack = aco_share_stack_new(0);
	request_handler = handler;

	/* Connect to dependencies */
	for (unsigned i = 0; i < ndependencies; i++) {
		unsigned id = dependencies[i];

		rc = unimsg_socket(&socks[nsocks]);
		if (rc) {
			fprintf(stderr, "Error creating unimsg socket: %s\n",
				strerror(-rc));
			exit(1);
		}

		rc = unimsg_connect(socks[nsocks], services[id].addr,
				    services[id].port);
		if (rc) {
			fprintf(stderr, "Error connecting to %s service: %s\n",
				services[id].name, strerror(-rc));
			exit(1);
		}

		downstream_socks[id] = socks[nsocks];
		nsocks++;

		DEBUG("Connected to %s service\n", services[id].name);
	}

	/* Listen for incoming connections */
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

		unsigned i;
		/* Handle downstream sockets */
		for (i = 1; i <= ndependencies; i++) {
			if (ready[i])
				handle_downstream(socks[i]);
		}

		/* Handle upstream sockets */
		for (; i < nsocks; i++) {
			if (ready[i]) {
				rc = handle_upstream(socks[i]);
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

		/* Handle new connections */
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

__unused
static void do_rpc(struct unimsg_shm_desc *desc, unsigned service,
		   size_t rr_size)
{
	int rc = unimsg_send(downstream_socks[service], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	DEBUG("[service] Sent request to %s service, yielding\n",
	      services[service].name);
	struct coroutine_args *args = aco_get_arg();
	args->down_desc = desc;
	aco_yield();
	DEBUG("[service] Resumed on response from %s service\n",
	      services[service].name);

	if (desc->size != sizeof(struct rpc) + rr_size) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

#endif /* __SERVICE__ */