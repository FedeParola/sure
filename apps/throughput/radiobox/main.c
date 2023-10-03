#include <getopt.h>
#include <unimsg/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>

#define UNIMSG_BUFFER_AVAILABLE (UNIMSG_BUFFER_SIZE - UNIMSG_BUFFER_HEADROOM)
#define DEFAULT_SIZE 64
#define SERVER_ADDR 1
#define SERVER_PORT 5000
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static unsigned opt_duration;
static unsigned opt_size = DEFAULT_SIZE;
static unsigned opt_connections;
static unsigned opt_http = 0;
static unsigned http_body_size;
static unsigned opt_buffers_reuse = 0;
static struct unimsg_shm_desc descs[UNIMSG_MAX_NSOCKS][UNIMSG_MAX_DESCS_BULK];
static unsigned ndescs;
static struct option long_options[] = {
	{"duration", required_argument, 0, 'd'},
	{"size", required_argument, 0, 's'},
	{"connections", required_argument, 0, 'c'},
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
		"  -d, --duration	Duration of the test in seconds\n"
		"  -s, --size		Size of the message in bytes (default %u)\n"
		"  -c, --connections	Number of client connections (if not specified or 0, behave as server)\n"
		"  -h, --http		Use HTTP payloads\n"
		"  -r, --buffers-reuse	Reuse shm buffers instead of reallocating on each rr\n",
		prog, DEFAULT_SIZE);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "d:s:c:bhr", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			opt_duration = atoi(optarg);
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
			opt_connections = atoi(optarg);
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

	if (opt_connections && !opt_duration) {
		fprintf(stderr, "Client must specify duration > 0\n");
		usage(argv[0]);
	}

	if (opt_http && !opt_connections) {
		if (opt_size < sizeof(http_resp)) {
			fprintf(stderr, "Message size (%u) too small to hold "
				"HTTP header (%lu)\n", opt_size,
				sizeof(http_resp));
			usage(argv[0]);
		}

		http_body_size = opt_size - sizeof(http_resp);
	}
}

static void client_send(struct unimsg_sock *s, unsigned id)
{
	int rc;

	if (!opt_buffers_reuse) {
		rc = unimsg_buffer_get(descs[id], ndescs); 
		if (rc) {
			fprintf(stderr, "Error getting shm buffer: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}
	}

	for (unsigned i = 0; i < ndescs; i++)
		*(char *)descs[id][i].addr = 0;
	if (opt_http)
		strcpy(descs[id][0].addr, http_req);

	rc = unimsg_send(s, descs[id], ndescs, 1);
	if (rc) {
		fprintf(stderr, "Error sending descs: %s\n", strerror(-rc));
		exit(1);
	}
}

static void client_recv(struct unimsg_sock *s, unsigned id, int nonblock)
{
	int rc;
	unsigned nrecv;

	nrecv = ndescs;
	rc = unimsg_recv(s, descs[id], &nrecv, nonblock);
	if (rc) {
		fprintf(stderr, "Error receiving descs: %s\n", strerror(-rc));
		exit(1);
	}
	if (nrecv < ndescs) {
		fprintf(stderr, "Received unexpected number of descs: %s\n",
			strerror(-rc));
		exit(1);
	}

	for (unsigned i = 0; i < ndescs; i++)
		*(char *)descs[id][i].addr = 0;

	if (!opt_buffers_reuse)
		unimsg_buffer_put(descs[id], ndescs);
}

static void client()
{
	int rc;
	struct unimsg_sock *socks[UNIMSG_MAX_NSOCKS];
	int ready[UNIMSG_MAX_NSOCKS];

	printf("I'm the client\n");

	ndescs = (opt_size - 1) / UNIMSG_BUFFER_AVAILABLE + 1;

	for (unsigned i = 0; i < opt_connections; i++) {
		struct unimsg_sock *s;

		rc = unimsg_socket(&s);
		if (rc) {
			fprintf(stderr, "Error creating unimsg socket: %s\n",
				strerror(-rc));
			exit(1);
		}

		rc = unimsg_connect(s, SERVER_ADDR, SERVER_PORT);
		if (rc) {
			fprintf(stderr, "Error connecting to server: %s\n",
				strerror(-rc));
			exit(1);
		}

		socks[i] = s;

		rc = unimsg_buffer_get(descs[i], ndescs); 
		if (rc) {
			fprintf(stderr, "Error getting shm buffer: %s\n",
				strerror(-rc));
			exit(1);
		}
	}
	printf("Sockets connected\n");

	printf("Running %u connections for %u seconds with %u bytes of "
	       "message\n", opt_connections, opt_duration, opt_size);

	unsigned long start = ukplat_monotonic_clock();

	for (unsigned i = 0; i < opt_connections; i++)
		client_send(socks[i], i);

	do {
		rc = unimsg_poll(socks, opt_connections, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		for (unsigned i = 0; i < opt_connections; i++) {
			if (ready[i]) {
				client_recv(socks[i], i, 1);
				client_send(socks[i], i);
			}
		}
	} while (ukplat_monotonic_clock() - start
		 < (unsigned long)opt_duration * 1000000000);

	for (unsigned i = 0; i < opt_connections; i++) {
		client_recv(socks[i], 0, i);
		if (opt_buffers_reuse)
			unimsg_buffer_put(descs[i], ndescs);
		unimsg_close(socks[i]);
	}

	printf("Sockets closed\n");
}

/* Returns 1 if the connection is closed */
static int do_server_rr(struct unimsg_sock *s)
{
	int rc;
	struct unimsg_shm_desc descs[UNIMSG_MAX_DESCS_BULK];
	unsigned nrecv;

	nrecv = UNIMSG_MAX_DESCS_BULK;
	rc = unimsg_recv(s, descs, &nrecv, 1);
	if (rc) {
		if (rc == -ECONNRESET) {
			return 1;
		} else {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}
	}

	for (unsigned i = 0; i < nrecv; i++)
		*(char *)descs[i].addr = 0;

	unsigned nsend = nrecv;
	if (opt_http) {
		nsend = (opt_size - 1) / UNIMSG_BUFFER_AVAILABLE + 1;
		if (nsend > nrecv) {
			rc = unimsg_buffer_get(&descs[nrecv], nsend - nrecv);
			if (rc) {
				fprintf(stderr, "Error getting shm buffer: "
					"%s\n",	strerror(-rc));
				ERR_CLOSE(s);
			}

		} else if (nsend < nrecv) {
			unimsg_buffer_put(&descs[nsend], nrecv - nsend);
		}

		sprintf(descs[0].addr, http_resp, http_body_size);
		for (unsigned i = 0; i < nsend - 1; i++)
			descs[i].size = UNIMSG_BUFFER_AVAILABLE;
		descs[nsend - 1].size = opt_size % UNIMSG_BUFFER_AVAILABLE;
	}

	rc = unimsg_send(s, descs, nsend, 1);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	return 0;
}

static void server()
{
	int rc;
	struct unimsg_sock *socks[UNIMSG_MAX_NSOCKS];
	int ready[UNIMSG_MAX_NSOCKS];
	unsigned nsocks = 1;

	printf("I'm the server\n");

	rc = unimsg_socket(&socks[0]);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		exit(1);
	}
	printf("Socket created\n");

	rc = unimsg_bind(socks[0], SERVER_PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", SERVER_PORT,
			strerror(-rc));
		exit(1);
	}
	printf("Socket bound\n");

	rc = unimsg_listen(socks[0]);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}
	printf("Socket listening\n");

	unsigned long rrs = 0;
	unsigned long start = 0;

	int started = 0;
	do {
		rc = unimsg_poll(socks, nsocks, ready);
		if (rc) {
			fprintf(stderr, "Error polling: %s\n", strerror(-rc));
			exit(1);
		}

		for (unsigned i = 1; i < nsocks; i++) {
			if (ready[i]) {
				rc = do_server_rr(socks[i]);
				if (rc == 1) {
					unimsg_close(socks[i]);
					unsigned j;
					for (j = i; j < nsocks - 1; j++) {
						socks[j] = socks[j + 1];
						ready[j] = ready[j + 1];
					}
					nsocks--;
				} else {
					rrs++;
				}
			}
		}

		if (ready[0]) {
			struct unimsg_sock *s;
			rc = unimsg_accept(socks[0], &s, 1);
			if (rc) {
				fprintf(stderr, "Error accepting connection: "
					"%s\n", strerror(-rc));
				exit(1);
			}
			
			if (!started) {
				printf("Handling connections\n");
				started = 1;
				start = ukplat_monotonic_clock();
			}

			socks[nsocks++] = s;
		}
	} while (nsocks > 1 || !started);

	unimsg_close(socks[0]);

	unsigned long stop = ukplat_monotonic_clock();

	printf("Sockets closed\n");

	printf("rrs=%lu\nrps=%lu\n", rrs, rrs * 1000000000 / (stop - start));
}

int main(int argc, char *argv[])
{
	parse_command_line(argc, argv);

	if (opt_connections)
		client();
	else
		server();

	return 0;
}