/*
 * Some sort of Copyright
 */

#ifndef __SERVICE_ASYNC__
#define __SERVICE_ASYNC__

#include "service.h"
#include "../libaco/aco.h"

#if ENABLE_DEBUG
#define DEBUG_SVC(co_id, fmt, ...)					\
	printf("[service (%d)] " fmt, co_id, ##__VA_ARGS__)
#else
#define DEBUG_SVC(...) (void)0
#endif

#define MAX_COROUTINES 16

struct coroutine {
	unsigned id;
	aco_t *handle;
	aco_share_stack_t *stack;
	/* Data of upstream request */
	struct unimsg_sock *up_sock;
	struct unimsg_shm_desc up_descs[UNIMSG_MAX_DESCS_BULK];
	unsigned up_ndescs;
	/* Data of downtream request */
	struct unimsg_shm_desc *down_desc;
};

static struct coroutine coroutines[MAX_COROUTINES];
static struct unimsg_sock *downstream_socks[NUM_SERVICES];
static handle_request_t request_handler;
static aco_t *main_co;
static unsigned available_cos[MAX_COROUTINES];
static unsigned n_available_cos;

static void coroutine_fn()
{
	struct coroutine *co = aco_get_arg();

	while (1) {
		DEBUG_SVC(co->id, "Handling request\n");

		unsigned nsend = co->up_ndescs;

		request_handler(co->up_descs, &nsend);

		int rc = unimsg_send(co->up_sock, co->up_descs, nsend, 0);
		if (rc) {
			unimsg_buffer_put(co->up_descs, co->up_ndescs > nsend ?
					  co->up_ndescs : nsend);
			if (rc) {
				fprintf(stderr, "Error sending desc: %s\n",
					strerror(-rc));
				_ERR_CLOSE(co->up_sock);
			}
		}

		DEBUG_SVC(co->id, "Sent response of %d buffers\n", nsend);

		/* Free excess buffers */
		if (nsend < co->up_ndescs) {
			unimsg_buffer_put(co->up_descs + nsend,
					  co->up_ndescs - nsend);
		}

		available_cos[n_available_cos++] = co->id;

		DEBUG_SVC(co->id, "Request handled\n");

		aco_yield();
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

	DEBUG_SVC(-1, "Received downstream response\n");

	/* Identify the coroutine */
	struct rpc *rpc = desc.addr;
	if (rpc->id >= MAX_COROUTINES) {
		fprintf(stderr, "Detected invalid coroutine id\n");
		exit(1);
	}
	struct coroutine *co = &coroutines[rpc->id];

	/* Copy args */
	*(co->down_desc) = desc;

	/* Resume coroutine */
	DEBUG_SVC(-1, "Resuming coroutine %u\n", co->id);
	aco_resume(co->handle);
}

static int handle_upstream(struct unimsg_sock *s)
{
	/* Find available coroutine */
	if (available_cos == 0) {
		fprintf(stderr, "No more available coroutines\n");
		_ERR_CLOSE(s);
	}
	struct coroutine *co = &coroutines[available_cos[n_available_cos - 1]];
	co->up_sock = s;
	co->up_ndescs = 1;

	int rc = unimsg_recv(s, co->up_descs, &co->up_ndescs, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG_SVC(-1, "Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving from upstream: %s\n",
			strerror(-rc));
		_ERR_CLOSE(s);
	}

	n_available_cos--;

	DEBUG_SVC(-1, "Received request of %d buffers\n", co->up_ndescs);
	DEBUG_SVC(-1, "Starting coroutine %u\n", co->id);

	aco_resume(co->handle);

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
	request_handler = handler;
	n_available_cos = 0;
	for (unsigned i = 0; i < MAX_COROUTINES; i++) {
		coroutines[i].id = i;
		coroutines[i].stack = aco_share_stack_new(0);
		coroutines[i].handle = aco_create(main_co, coroutines[i].stack,
						  0, coroutine_fn,
						  &coroutines[i]);
		available_cos[i] = i;
	}
	n_available_cos = MAX_COROUTINES;

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

		DEBUG_SVC(-1, "Connected to %s service\n", services[id].name);
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
	DEBUG_SVC(-1, "Waiting for incoming connections...\n");

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

			DEBUG_SVC(-1, "New client connected\n");
		}
	}
}

__unused
static void do_rpc(struct unimsg_shm_desc *desc, unsigned service,
		   size_t rr_size)
{
	struct rpc *rpc = desc->addr;
	struct coroutine *co = aco_get_arg();
	rpc->id = co->id;

	int rc = unimsg_send(downstream_socks[service], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	DEBUG_SVC(co->id, "Sent request to %s service, yielding\n",
		  services[service].name);
	co->down_desc = desc;
	aco_yield();
	DEBUG_SVC(co->id, "Resumed on response from %s service\n",
		  services[service].name);

	if (desc->size != sizeof(struct rpc) + rr_size) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

#endif /* __SERVICE_ASYNC__ */