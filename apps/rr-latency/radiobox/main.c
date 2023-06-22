#include <getopt.h>
#include <h2os/net.h>
#include <h2os/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>

#define SERVER_VM_ID 0
#define SERVER_PORT 5000
#define CLIENT_VM_ID 1
#define ERR_CLOSE(s) ({ h2os_sock_close(s); exit(1); })
#define ERR_PUT(desc, s) ({ h2os_buffer_put(&(desc)); ERR_CLOSE(s); })

static unsigned opt_iterations = 0;
static unsigned opt_size = 0;
static int opt_client = 0;
static int opt_busy_poll = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"busy-poll", optional_argument, 0, 'b'},
	{0, 0, 0, 0}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of requests-responses to exchange\n"
		"  -s, --size		Size of the message in bytes\n"
		"  -c, --client		Behave as client (default is server)\n"
		"  -b, --busy-poll	Use busy polling (non-blocking sockets)\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cb", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		case 's':
			opt_size = atoi(optarg);
			/* Since we read/write the message as 8 byte words,
			 * round size to the upper multiple of 8
			 */
			if (opt_size & 7)
				opt_size += 8;
			opt_size &= ~7;
			break;
		case 'c':
			opt_client = 1;
			break;
		case 'b':
			opt_busy_poll = 1;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (opt_iterations == 0 || opt_size == 0) {
		fprintf(stderr, "Iterations and size are required parameteres "
			"and must be > 0\n");
		usage(argv[0]);
	}
}

static void client(struct h2os_sock *s)
{
	int rc;
	struct h2os_shm_desc desc;
	unsigned long *msg;

	printf("I'm the client\n");

	rc = h2os_sock_connect(s, SERVER_VM_ID, SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket connected\n");

	printf("Sending %u requests of %u bytes\n", opt_iterations, opt_size);

	unsigned long start = ukplat_monotonic_clock();

	for (unsigned long i = 0; i < opt_iterations; i++) {
		rc = h2os_buffer_get(&desc); 
		if (rc) {
			fprintf(stderr, "Error getting shm buffer: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		msg = desc.addr;
		for (unsigned j = 0; j < opt_size / 8; j++)
			msg[j] = i;

		if (opt_busy_poll)
			while ((rc = h2os_sock_send(s, &desc)) == -EAGAIN);
		else
			rc = h2os_sock_send(s, &desc);
		if (rc) {
			fprintf(stderr, "Error sending buffer %p: %s\n",
				desc.addr, strerror(-rc));
			ERR_PUT(desc, s);
		}

		if (opt_busy_poll)
			while ((rc = h2os_sock_recv(s, &desc)) == -EAGAIN);
		else
			rc = h2os_sock_recv(s, &desc);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		msg = desc.addr;
		for (unsigned j = 0; j < opt_size / 8; j++)
			if (msg[j] != i + 1) {
				fprintf(stderr, "Received unexpected message "
					"%lu\n", msg[j]);
				ERR_PUT(desc, s);
			}

		h2os_buffer_put(&desc);
	}

	unsigned long stop = ukplat_monotonic_clock();

	printf("Total time: %lu ns, average rr latency: %lu ns\n",
		stop - start, (stop - start) / opt_iterations);

	h2os_sock_close(s);
	printf("Socket closed\n");
}

static void server(struct h2os_sock *s)
{
	int rc;
	struct h2os_shm_desc desc;
	unsigned long *msg;

	printf("I'm the server\n");

	rc = h2os_sock_bind(s, SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", SERVER_PORT,
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket bound\n");

	rc = h2os_sock_listen(s);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket listening\n");

	struct h2os_sock *cs;
	if (opt_busy_poll)
		while ((rc = h2os_sock_accept(s, &cs)) == -EAGAIN);
	else
		rc = h2os_sock_accept(s, &cs);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Connection accepted\n");

	h2os_sock_close(s);
	printf("Listening socket closed\n");

	s = cs;

	printf("Handling requests\n");

	for (unsigned i = 0; i < opt_iterations; i++) {
		if (opt_busy_poll)
			while ((rc = h2os_sock_recv(s, &desc)) == -EAGAIN);
		else
			rc = h2os_sock_recv(s, &desc);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		msg = desc.addr;
		for (unsigned j = 0; j < opt_size / 8; j++)
			msg[j]++;

		if (opt_busy_poll)
			while ((rc = h2os_sock_send(s, &desc)) == -EAGAIN);
		else
			rc = h2os_sock_send(s, &desc);
		if (rc) {
			fprintf(stderr, "Error sending buffer %p: %s\n",
				desc.addr, strerror(-rc));
			ERR_PUT(desc, s);
		}
	}

	printf("Test terminated\n");

	h2os_sock_close(s);
	printf("Socket closed\n");
}

int main(int argc, char *argv[])
{
	int rc;
	struct h2os_sock *s;

	parse_command_line(argc, argv);

	rc = h2os_sock_create(&s, H2OS_SOCK_CONNECTED, opt_busy_poll);
	if (rc) {
		fprintf(stderr, "Error creating h2os socket: %s\n",
			strerror(-rc));
		return 1;
	}
	printf("Socket created\n");

	if (opt_client)
		client(s);
	else
		server(s);

	return 0;
}