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

static int handle_socket(struct unimsg_sock *s, handle_request_t handle_request)
{
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned nrecv = 1;

	int rc = unimsg_recv(s, descs, &nrecv, 0);
	if (rc == -ECONNRESET) {
		unimsg_close(s);
		DEBUG_SVC("Connection closed\n");
		return 1;
	} else if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		_ERR_CLOSE(s);
	}

	DEBUG_SVC("Received request of %d buffers\n", nrecv);

	unsigned nsend = nrecv;
	handle_request(descs, &nsend);

	rc = unimsg_send(s, descs, nsend, 0);
	if (rc) {
		unimsg_buffer_put(descs, nrecv > nsend ? nrecv : nsend);
		if (rc == -ECONNRESET) {
			unimsg_close(s);
			DEBUG_SVC("Connection closed\n");
			return 1;
		} else if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			_ERR_CLOSE(s);
		}
	}

	DEBUG_SVC("Sent response of %d buffers\n", nsend);

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

	DEBUG_SVC("Waiting for incoming connections...\n");

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

			DEBUG_SVC("New client connected\n");
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
		fprintf(stderr, "Expected %lu B, got %u B from %s service\n",
			sizeof(struct rpc) + rr_size, desc->size,
			services[service].name);
		exit(1);
	}
}

#endif /* __SERVICE_SYNC__ */