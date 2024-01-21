#include <getopt.h>
#include <unimsg/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>

#define UNIMSG_BUFFER_AVAILABLE						\
	(UNIMSG_BUFFER_SIZE - UNIMSG_BUFFER_HEADROOM - 68)
#define DEFAULT_SIZE 64
#define DEFAULT_WARMUP 0
#define DEFAULT_DELAY 0
#define SERVER_ADDR 0x0100000a /* 10.0.0.1 */
#define SERVER_PORT 5000
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
static unsigned opt_http = 0;
static unsigned http_body_size;
static unsigned opt_buffers_reuse = 0;
static struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
static unsigned ndescs;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"busy-poll", optional_argument, 0, 'b'},
	{"warmup", optional_argument, 0, 'w'},
	{"delay", optional_argument, 0, 'd'},
	{"http", optional_argument, 0, 'h'},
	{"buffers-reuse", optional_argument, 0, 'r'},
	{0, 0, 0, 0}
};

static char http_req[] = "GET / HTTP/1.1\r\n"
			 "Host: localhost\r\n"
			 "User-Agent: custom-client/1.0.0\r\n"
			 "Accept: */*\r\n"
			 "\r\n";
/* The Content-Length field is expected to always occupy 4B, hence
 * sizeof(http_resp) returns the correct length of the string after the
 * placeholder has been replaced (%4u only occupies 3 chars, but sizeof also
 * accounts for the trailing \0)
 */
static char http_resp[] = "HTTP/1.1 200 OK\r\n"
			  "Server: custom-server/1.0.0\r\n"
			  "Date: Thu, 07 Sep 2023 20:57:10 GMT\r\n" /* current datetime */
			  "Content-Type: text/html\r\n"
			  "Content-Length: %4u\r\n" /* body length */
			  "Connection: keep-alive\r\n"
			  "\r\n";
			  /* "%s"; omitted body for now */

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
		"  -d, --delay		Delay between consecutive requests in ms (default %u)\n"
		"  -h, --http		Use HTTP payloads\n"
		"  -r, --buffers-reuse	Reuse shm buffers instead of reallocating on each rr\n",
		prog, DEFAULT_SIZE, DEFAULT_WARMUP, DEFAULT_DELAY);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cbw:d:hr", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		case 's':
			opt_size = atoi(optarg);
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
		case 'h':
			opt_http = 1;
			break;
		case 'r':
			opt_buffers_reuse = 1;
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

	if (opt_http && !opt_client) {
		if (opt_size < sizeof(http_resp)) {
			fprintf(stderr, "Message size (%u) too small to hold "
				"HTTP header (%lu)\n", opt_size,
				sizeof(http_resp));
			usage(argv[0]);
		}

		http_body_size = opt_size - sizeof(http_resp);
	}
}

static void do_client_rr(struct unimsg_sock *s)
{
	int rc;
	unsigned nrecv;
#ifdef ADDITIONAL_STATS
	unsigned long start, stop;
#endif

	if (!opt_buffers_reuse) {
		rc = unimsg_buffer_get(descs, ndescs); 
		if (rc) {
			fprintf(stderr, "Error getting shm buffer: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}
	}

	for (unsigned i = 0; i < ndescs; i++) {
		*(char *)descs[i].addr = 0;
		descs[i].size = UNIMSG_BUFFER_AVAILABLE;
	}
	descs[ndescs - 1].size = opt_size % UNIMSG_BUFFER_AVAILABLE;
	if (descs[ndescs - 1].size == 0)
		descs[ndescs - 1].size = UNIMSG_BUFFER_AVAILABLE;
	if (opt_http) {
		strcpy(descs[0].addr, http_req);
		descs[0].size = sizeof(http_req);
	}

	do {
		STORE_TIME(start);
		rc = unimsg_send(s, descs, ndescs, opt_busy_poll);
		STORE_TIME(stop);
	} while (opt_busy_poll && rc == -EAGAIN);
	if (rc) {
		fprintf(stderr, "Error sending descs: %s\n", strerror(-rc));
		ERR_PUT(descs, ndescs, s);
	}

#ifdef ADDITIONAL_STATS
	if (++iterations_count > opt_warmup)
		send_time += stop - start;
#endif

	unsigned rdescs = 0, rsize = 0;
	do {
		do {
			STORE_TIME(start);
			nrecv = UNIMSG_MAX_DESCS_BULK;
			rc = unimsg_recv(s, &descs[rdescs], &nrecv,
					 opt_busy_poll);
			STORE_TIME(stop);

			if (!rc) {
				for (unsigned i = 0; i < nrecv; i++)
					rsize += descs[rdescs + i].size;
				rdescs += nrecv;
			}
		} while (opt_busy_poll && rc == -EAGAIN);
	} while (!opt_http && !rc && rsize < opt_size);
	if (rc) {
		fprintf(stderr, "Error receiving descs: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

#ifdef ADDITIONAL_STATS
	if (iterations_count > opt_warmup)
		recv_time += stop - start;
#endif

	for (unsigned i = 0; i < rdescs; i++)
		*(char *)descs[i].addr = 0;

	if (!opt_buffers_reuse)
		unimsg_buffer_put(descs, rdescs);
	else if (rdescs > ndescs)
		unimsg_buffer_put(&descs[ndescs], rdescs - ndescs);
}

static void client(struct unimsg_sock *s)
{
	int rc;

	printf("I'm the client\n");

	rc = unimsg_connect(s, SERVER_ADDR, SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}
	printf("Socket connected\n");

	ndescs = (opt_size - 1) / UNIMSG_BUFFER_AVAILABLE + 1;
	if (opt_buffers_reuse) {
		rc = unimsg_buffer_get(descs, ndescs); 
		if (rc) {
			fprintf(stderr, "Error getting shm buffer: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}
	}

	if (opt_warmup) {
		printf("Performing %u warmup RRs...\n", opt_warmup);
		for (unsigned long i = 0; i < opt_warmup; i++)
			do_client_rr(s);
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

		do_client_rr(s);

		if (opt_delay) {
			latency = ukplat_monotonic_clock() - start;
			total += latency;
			printf("%lu=%lu\n", i, latency);
		}
	}

	if (!opt_delay)
		total = ukplat_monotonic_clock() - start;

	if (opt_buffers_reuse)
		unimsg_buffer_put(descs, ndescs);

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
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned nrecv;

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
		unsigned rdescs = 0, rsize = 0;
		do {
			do {
				STORE_TIME(start);
				nrecv = UNIMSG_MAX_DESCS_BULK;
				rc = unimsg_recv(s, &descs[rdescs], &nrecv,
						opt_busy_poll);
				STORE_TIME(stop);

				if (!rc) {
					for (unsigned i = 0; i < nrecv; i++)
						rsize += descs[rdescs + i].size;
					rdescs += nrecv;
				}
			} while (opt_busy_poll && rc == -EAGAIN);
		} while (!opt_http && !rc && rsize < opt_size);
		if (rc == -ECONNRESET) {
			break;
		} else if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		for (unsigned i = 0; i < rdescs; i++)
			*(char *)descs[i].addr = 0;

		unsigned nsend = rdescs;
		if (opt_http) {
			nsend = (opt_size - 1) / UNIMSG_BUFFER_AVAILABLE + 1;
			if (nsend > rdescs) {
				rc = unimsg_buffer_get(&descs[rdescs],
						       nsend - rdescs);
				if (rc) {
					fprintf(stderr, "Error getting shm "
						"buffer: %s\n",	strerror(-rc));
					ERR_CLOSE(s);
				}

			} else if (nsend < rdescs) {
				unimsg_buffer_put(&descs[nsend],
						  rdescs - nsend);
			}

			sprintf(descs[0].addr, http_resp, http_body_size);
			for (unsigned i = 0; i < nsend - 1; i++)
				descs[i].size = UNIMSG_BUFFER_AVAILABLE;
			descs[nsend - 1].size =
				opt_size % UNIMSG_BUFFER_AVAILABLE;
			if (descs[nsend - 1].size == 0)
				descs[nsend - 1].size = UNIMSG_BUFFER_AVAILABLE;
		}

		do {
			STORE_TIME(start);
			rc = unimsg_send(s, descs, nsend, opt_busy_poll);
			STORE_TIME(stop);
		} while (opt_busy_poll && rc == -EAGAIN);
		if (rc == -ECONNRESET) {
			unimsg_buffer_put(descs, nsend);
			break;
		} else if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			ERR_PUT(descs, nsend, s);
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