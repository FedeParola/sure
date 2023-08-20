#include <getopt.h>
#include <unimsg/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>

#define DEFAULT_SIZE 64
#define DEFAULT_WARMUP 0
#define DEFAULT_DELAY 0
#define SERVER_VM_ID 0
#define SERVER_PORT 5000
#define MAX_MSG_SIZE 8192
#define CLIENT_VM_ID 1
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})
#ifdef ADDITIONAL_STATS
#define STORE_TIME(var) ({ var = ukplat_monotonic_clock(); })
#else
#define STORE_TIME(var)
#endif

#ifdef ADDITIONAL_STATS
static unsigned long send_time;
static unsigned long recv_time;
static unsigned long iterations_count;
#endif
static unsigned opt_iterations = 0;
static unsigned opt_size = DEFAULT_SIZE;
static int opt_client = 0;
static int opt_busy_poll = 0;
static unsigned opt_warmup = DEFAULT_WARMUP;
static unsigned opt_delay = DEFAULT_DELAY;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"busy-poll", optional_argument, 0, 'b'},
	{"warmup", optional_argument, 0, 'w'},
	{"delay", optional_argument, 0, 'd'},
	{0, 0, 0, 0}
};

int usleep(unsigned usec);

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of requests-responses to exchange\n"
		"  -s, --size		Size of the message in bytes (default %u)\n"
		"  -c, --client		Behave as client (default is server)\n"
		"  -b, --busy-poll	Use busy polling (non-blocking sockets)\n"
		"  -w, --warmup		Number of warmup iterations (default %u)\n"
		"  -d, --delay		Delay between consecutive requests in ms (default %u)\n",
		prog, DEFAULT_SIZE, DEFAULT_WARMUP, DEFAULT_DELAY);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cbw:d:", long_options,
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
		case 'w':
			opt_warmup = atoi(optarg);
			break;
		case 'd':
			opt_delay = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (opt_size == 0) {
		fprintf(stderr, "Size must be > 0\n");
		usage(argv[0]);
	}

	if (opt_client && !opt_iterations) {
		fprintf(stderr, "Client must specify iterations > 0\n");
		usage(argv[0]);
	}
}

static void do_client_rr(struct unimsg_sock *s, unsigned long val)
{
	int rc;
	unsigned long msg[MAX_MSG_SIZE / sizeof(unsigned long)];
#ifdef ADDITIONAL_STATS
	unsigned long start, stop;
#endif

	do {
		STORE_TIME(start);
		rc = unimsg_send(s, msg, opt_size, opt_busy_poll);
		STORE_TIME(stop);
	} while (opt_busy_poll && rc == -EAGAIN);
	if (rc) {
		fprintf(stderr, "Error sending message: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

#ifdef ADDITIONAL_STATS
	if (++iterations_count > opt_warmup)
		send_time += stop - start;
#endif

	unsigned long len = sizeof(msg);
	do {
		STORE_TIME(start);
		rc = unimsg_recv(s, msg, &len, opt_busy_poll);
		STORE_TIME(stop);
	} while (opt_busy_poll && rc == -EAGAIN);
	if (rc) {
		fprintf(stderr, "Error receiving descs: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}
	if (len < opt_size) {
		fprintf(stderr, "Received unexpected message\n");
		ERR_CLOSE(s);
	}

#ifdef ADDITIONAL_STATS
	if (iterations_count > opt_warmup)
		recv_time += stop - start;
#endif
}

static void client(struct unimsg_sock *s)
{
	int rc;

	printf("I'm the client\n");

	rc = unimsg_connect(s, SERVER_VM_ID, SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket connected\n");

	if (opt_warmup) {
		printf("Performing %u warmup RRs...\n", opt_warmup);
		for (unsigned long i = 0; i < opt_warmup; i++)
			do_client_rr(s, i);
	}

	printf("Sending %u requests of %u bytes with %u ms of delay\n",
	       opt_iterations, opt_size, opt_delay);

	unsigned long start = 0, total = 0, latency;

	if (!opt_delay)
		start = ukplat_monotonic_clock();

	for (unsigned long i = 0; i < opt_iterations; i++) {
		if (opt_delay) {
			usleep(opt_delay * 1000);
			start = ukplat_monotonic_clock();
		}

		do_client_rr(s, i);

		if (opt_delay) {
			latency = ukplat_monotonic_clock() - start;
			total += latency;
			printf("%lu=%lu\n", i, latency);
		}
	}

	if (!opt_delay)
		total = ukplat_monotonic_clock() - start;

	unimsg_close(s);
	printf("Socket closed\n");

	printf("total-time=%lu\nrr-latency=%lu\n", total,
	       total / opt_iterations);

#ifdef ADDITIONAL_STATS
	printf("Average send time %lu ns\n",
	       send_time / (iterations_count - opt_warmup));
	printf("Average recv time %lu ns\n",
	       recv_time / (iterations_count - opt_warmup));
#endif
}

static void server(struct unimsg_sock *s)
{
	int rc;
	unsigned long msg[MAX_MSG_SIZE / sizeof(unsigned long)];

	printf("I'm the server\n");

	rc = unimsg_bind(s, SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", SERVER_PORT,
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket bound\n");

	rc = unimsg_listen(s);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket listening\n");

	struct unimsg_sock *cs;
	rc = unimsg_accept(s, &cs, 0);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Connection accepted\n");

	unimsg_close(s);
	printf("Listening socket closed\n");

	s = cs;

	printf("Handling requests\n");

	/* Handle requests until the connection is closed by the client */
	for (;;) {
#ifdef ADDITIONAL_STATS
		unsigned long start, stop;
#endif
		unsigned long len = MAX_MSG_SIZE;
		do {
			STORE_TIME(start);
			rc = unimsg_recv(s, msg, &len, opt_busy_poll);
			STORE_TIME(stop);
		} while (opt_busy_poll && rc == -EAGAIN);
		if (rc == -ECONNRESET) {
			break;
		} else if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		do {
			STORE_TIME(start);
			rc = unimsg_send(s, msg, len, opt_busy_poll);
			STORE_TIME(stop);
		} while (opt_busy_poll && rc == -EAGAIN);
		if (rc == -ECONNRESET) {
			break;
		} else if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

#ifdef ADDITIONAL_STATS
		send_time += stop - start;
		iterations_count++;
#endif
	}

	printf("Test terminated\n");

	unimsg_close(s);
	printf("Socket closed\n");

#ifdef ADDITIONAL_STATS
	printf("Average send time %lu ns\n", send_time / iterations_count);
	printf("Average recv time %lu ns\n", recv_time / iterations_count);
#endif
}

int main(int argc, char *argv[])
{
	int rc;
	struct unimsg_sock *s;

	parse_command_line(argc, argv);

	rc = unimsg_socket(&s);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
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