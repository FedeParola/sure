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

#define MAX_COROUTINES 32

struct coroutine {
	unsigned id;
	aco_t *handle;
	aco_share_stack_t *stack;
	/* Data of upstream request */
	int up_sock;
	struct unimsg_shm_desc up_desc;
	/* Data of downtream request */
	struct unimsg_shm_desc *down_desc;
};

static struct coroutine coroutines[MAX_COROUTINES];
static int downstream_socks[NUM_SERVICES];
static handle_request_t request_handler;
static aco_t *main_co;
static unsigned available_cos[MAX_COROUTINES];
static unsigned n_available_cos;
static int disable_upstream;

static void coroutine_fn()
{
	struct coroutine *co = aco_get_arg();

	while (1) {
		DEBUG_SVC(co->id, "Handling request\n");

		request_handler(&co->up_desc);

		int rc = unimsg_send(co->up_sock, &co->up_desc, 1, 0);
		if (rc) {
			unimsg_buffer_put(&co->up_desc, 1);
			if (rc) {
				fprintf(stderr, "Error sending desc: %s\n",
					strerror(-rc));
				_ERR_CLOSE(co->up_sock);
			}
		}

		DEBUG_SVC(co->id, "Sent response\n");

		if (n_available_cos == 0) {
			DEBUG_SVC(co->id , "Enabling upstream reception on "
				  "coroutine termination\n");
			disable_upstream = 0;
		}
		available_cos[n_available_cos++] = co->id;

		DEBUG_SVC(co->id, "Request handled\n");

		aco_yield();
	}

	aco_exit();
}

static void handle_downstream(int s, struct pending_buffer *pending)
{
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned ndescs = UNIMSG_MAX_DESCS_BULK;

	int rc = unimsg_recv(s, descs, &ndescs, 0);
	if (rc) {
		fprintf(stderr, "Error receiving from downstream: %s\n",
			strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG_SVC(-1, "Received %u descs from downstream\n", ndescs);

	unsigned current = 0;
	while (current < ndescs) {
		if (process_desc(pending, &descs[current])) {
			DEBUG_SVC(-1, "Received downstream response\n");

			/* Identify the coroutine */
			struct rpc *rpc = pending->desc.addr;
			if (rpc->id >= MAX_COROUTINES) {
				fprintf(stderr, "Detected invalid coroutine "
					"id\n");
				exit(1);
			}
			struct coroutine *co = &coroutines[rpc->id];

			/* Copy args */
			*(co->down_desc) = pending->desc;

			/* Clear pending */
			pending->desc.addr = 0;
			pending->desc.size = 0;
			pending->expected_sz = 0;

			/* Resume coroutine */
			DEBUG_SVC(-1, "Resuming coroutine %u\n", co->id);
			aco_resume(co->handle);
		}

		if (descs[current].size == 0)
			current++;
	}

	DEBUG_SVC(-1, "Done processing bulk\n");
}

#if UPSTREAM_HTTP
#define handle_upstream handle_upstream_http
#else
#define handle_upstream handle_upstream_grpc
#endif

__unused
static int handle_upstream_http(int s, struct pending_buffer *pending __unused)
{
	/* Find available coroutine */
	if (n_available_cos == 0) {
		DEBUG_SVC(-1 , "Disabling upstream reception for lack of "
			  "coroutines\n");
		disable_upstream = 1;
		return 2;
	}
	struct coroutine *co = &coroutines[available_cos[n_available_cos - 1]];
	co->up_sock = s;

	unsigned ndescs = 1;
	int rc = unimsg_recv(s, &co->up_desc, &ndescs, 0);
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

	DEBUG_SVC(-1, "Received request\n");
	DEBUG_SVC(-1, "Starting coroutine %u\n", co->id);

	aco_resume(co->handle);

	return 0;
}

__unused
static int handle_upstream_grpc(int s, struct pending_buffer *pending)
{
	/* Check available coroutine */
	if (n_available_cos == 0) {
		DEBUG_SVC(-1 , "Disabling upstream reception for lack of "
			  "coroutines\n");
		disable_upstream = 1;
		return 2;
	}

	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned ndescs = UNIMSG_MAX_DESCS_BULK;
	int rc = unimsg_recv(s, descs, &ndescs, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG_SVC(-1, "Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving from upstream: %s\n",
			strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG_SVC(-1, "Received %u descs from upstream\n", ndescs);

	unsigned current = 0;
	while (current < ndescs) {
		/* Check available coroutine */
		if (n_available_cos == 0) {
			fprintf(stderr, "No more available coroutines but data "
				"is already pending");
			exit(1);
		}

		if (process_desc(pending, &descs[current])) {
			struct coroutine *co =
				&coroutines[available_cos[n_available_cos - 1]];
			n_available_cos--;

			co->up_desc = pending->desc;
			co->up_sock = s;

			/* Clear pending */
			pending->desc.addr = 0;
			pending->desc.size = 0;
			pending->expected_sz = 0;

			/* Validate message */
			struct rpc *rpc = co->up_desc.addr;
			if (co->up_desc.size != get_rpc_size(rpc->command)) {
				fprintf(stderr, "Expected %lu B, got %u B from "
					"upstream\n",
					get_rpc_size(rpc->command),
					co->up_desc.size);
				exit(1);
			}

			DEBUG_SVC(-1, "Received request\n");
			DEBUG_SVC(-1, "Starting coroutine %u\n", co->id);

			aco_resume(co->handle);
		}

		if (descs[current].size == 0)
			current++;
	}

	DEBUG_SVC(-1, "Done processing bulk\n");

	return 0;
}

static void run_service(unsigned id, handle_request_t handler,
			int *dependencies, unsigned ndependencies)
{
	int rc;
	int socks[UNIMSG_MAX_NSOCKS];
	struct pending_buffer pending_buffers[UNIMSG_MAX_NSOCKS];
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

		socks[nsocks] = unimsg_socket();
		if (socks[nsocks] < 0) {
			fprintf(stderr, "Error creating unimsg socket: %s\n",
				strerror(-socks[nsocks]));
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
	socks[0] = unimsg_socket();
	if (socks[0] < 0) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-socks[0]));
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

	disable_upstream = 0;

	memset(pending_buffers, 0, sizeof(pending_buffers));

	while (1) {
		unsigned npoll = disable_upstream ? ndependencies + 1 : nsocks;
		rc = unimsg_poll(socks, npoll, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		unsigned i;
		/* Handle downstream sockets */
		for (i = 1; i <= ndependencies; i++) {
			if (ready[i]) {
				handle_downstream(socks[i],
						  &pending_buffers[i]);
			}
		}

		/* Handle upstream sockets, if enabled */
		for (; i < npoll; i++) {
			if (ready[i]) {
				rc = handle_upstream(socks[i],
						     &pending_buffers[i]);
				if (rc == 1) {
					/* The socket was closed, remove it from
					 * the list and shift all other sockets
					 */
					nsocks--;
					npoll--;
					for (unsigned j = i; j < nsocks; j++) {
						socks[j] = socks[j + 1];
						ready[j] = ready[j + 1];
					}

				} else if (rc == 2) {
					/* We cannot receive more due to lack of
					 * coroutines
					 */
					/* TODO: to be fair, start receiveing
					 * from the next socket on the next
					 * round
					 */
					break;
				}
			}
		}

		/* Handle new connections */
		if (ready[0]) {
			if (nsocks == UNIMSG_MAX_NSOCKS) {
				fprintf(stderr, "Reached max number of "
					"connections\n");
				exit(1);
			}

			int s = unimsg_accept(socks[0], 1);
			if (s < 0) {
				fprintf(stderr, "Error accepting connection: "
					"%s\n", strerror(-s));
				_ERR_CLOSE(socks[0]);
			}

			socks[nsocks++] = s;

			DEBUG_SVC(-1, "New client connected\n");
		}
	}
}

__unused
static void do_rpc(struct unimsg_shm_desc *desc, unsigned service)
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

	rpc = desc->addr;
	if (desc->size != get_rpc_size(rpc->command)) {
		fprintf(stderr, "Expected %lu B, got %u B from %s service\n",
			get_rpc_size(rpc->command), desc->size,
			services[service].name);
		exit(1);
	}
}

#endif /* __SERVICE_ASYNC__ */
