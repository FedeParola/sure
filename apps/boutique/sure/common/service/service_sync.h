/*
 * Some sort of Copyright
 */

#ifndef __SERVICE_SYNC__
#define __SERVICE_SYNC__

#include "service.h"

#if ENABLE_DEBUG
#define DEBUG_SVC(fmt, ...)						\
	printf("[service] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_SVC(...) (void)0
#endif

static int handle_socket(int s, handle_request_t handle_request,
			 struct pending_buffer *pending)
{
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned ndescs = UNIMSG_MAX_DESCS_BULK;

	int rc = unimsg_recv(s, descs, &ndescs, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG_SVC("Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG_SVC("Received %u descs from upstream\n", ndescs);

	unsigned current = 0;
	while (current < ndescs) {
		if (process_desc(pending, &descs[current])) {
			/* Validate message */
			struct rpc *rpc = pending->desc.addr;
			if (pending->desc.size != get_rpc_size(rpc->command)) {
				fprintf(stderr, "Expected %lu B, got %u B from "
					"upstream\n",
					get_rpc_size(rpc->command),
					pending->desc.size);
				exit(1);
			}

			DEBUG_SVC("Received request\n");

			handle_request(&pending->desc);

			rc = unimsg_send(s, &pending->desc, 1, 0);
			if (rc) {
				unimsg_buffer_put(descs, ndescs);
				if (rc == -ECONNRESET) {
					unimsg_close(s);
					DEBUG_SVC("Connection closed\n");
					return 1;
				} else if (rc) {
					fprintf(stderr, "Error sending desc: "
						"%s\n", strerror(-rc));
					_ERR_CLOSE(s);
				}
			}

			DEBUG_SVC("Sent response\n");

			/* Clear pending */
			pending->desc.addr = 0;
			pending->desc.size = 0;
			pending->expected_sz = 0;
		}

		if (descs[current].size == 0)
			current++;
	}

	DEBUG_SVC("Done processing bulk\n");

	return 0;
}

static void run_service(unsigned id, handle_request_t handle_request)
{
	int rc;
	int socks[UNIMSG_MAX_NSOCKS];
	struct pending_buffer pending_buffers[UNIMSG_MAX_NSOCKS];
	int ready[UNIMSG_MAX_NSOCKS];
	unsigned nsocks = 1;

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

	memset(pending_buffers, 0, sizeof(pending_buffers));

	DEBUG_SVC("Waiting for incoming connections...\n");

	while (1) {
		rc = unimsg_poll(socks, nsocks, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		for (unsigned i = 1; i < nsocks; i++) {
			if (ready[i]) {
				rc = handle_socket(socks[i], handle_request,
						   &pending_buffers[i]);
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

			DEBUG_SVC("New client connected\n");
		}
	}
}

static int socks[NUM_SERVICES];

__unused
static void do_rpc(struct unimsg_shm_desc *desc, unsigned service)
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

	struct rpc *rpc = desc->addr;
	if (desc->size != get_rpc_size(rpc->command)) {
		fprintf(stderr, "Expected %lu B, got %u B from %s service\n",
			get_rpc_size(rpc->command), desc->size,
			services[service].name);
		exit(1);
	}
}

#endif /* __SERVICE_SYNC__ */
