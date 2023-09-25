#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#define DEFAULT_SIZE 64
#define DEFAULT_WARMUP 0
#define DEFAULT_DELAY 0
#define SERVER_ADDR 0x0100000a /* Already in nbo */
#define LOCALHOST 0x0100007f /* localhost, already in nbo */
#define DEFAULT_PORT 5000
#define MAX_MSG_SIZE 16384
#define SOCKET_PATH "/tmp/rr-latency.sock"
#define SOCKMAP_PATH "/sys/fs/bpf/sockmap"
#define ERR_CLOSE(s) ({ close(s); exit(1); })
#define ERR_UNPIN(s) ({ if (opt_sk_msg) unlink(SOCKMAP_PATH); ERR_CLOSE(s); })
#ifdef ADDITIONAL_STATS
#define STORE_TIME(var) ({ clock_gettime(CLOCK_MONOTONIC, &var); })
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
static int opt_unix = 0;
static int opt_sk_msg = 0;
static int opt_server_addr = SERVER_ADDR;
static unsigned opt_warmup = DEFAULT_WARMUP;
static unsigned opt_delay = DEFAULT_DELAY;
static unsigned opt_http = 0;
static unsigned http_body_size;
static uint16_t opt_port = DEFAULT_PORT;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"busy-poll", optional_argument, 0, 'b'},
	{"unix", optional_argument, 0, 'u'},
	{"sk-msg", optional_argument, 0, 'm'},
	{"localhost", optional_argument, 0, 'l'},
	{"warmup", optional_argument, 0, 'w'},
	{"delay", optional_argument, 0, 'd'},
	{"http", optional_argument, 0, 'h'},
	{"port", optional_argument, 0, 'p'},
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

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of requests-responses to exchange\n"
		"  -s, --size		Size of the message in bytes (default %u)\n"
		"  -c, --client		Behave as client (default is server)\n"
		"  -b, --busy-poll	Use busy polling (non-blocking sockets)\n"
		"  -u, --unix		Use AF_UNIX sockets\n"
		"  -m, --sk-msg		Use SK_MSG acceleration (TCP sockets only)\n"
		"  -l, --localhost	Run test on localhost\n"
		"  -w, --warmup		Number of warmup iterations (default %u)\n"
		"  -d, --delay		Delay between consecutive requests in ms (default %u)\n"
		"  -h, --http		Use HTTP payloads\n"
		"  -p, --port		Port to listen on / connect to (default %u)\n",
		prog, DEFAULT_SIZE, DEFAULT_WARMUP, DEFAULT_DELAY,
		DEFAULT_PORT);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cbumlw:d:hp:", long_options,
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
		case 'u':
			opt_unix = 1;
			break;
		case 'm':
			opt_sk_msg = 1;
			break;
		case 'l':
			opt_server_addr = LOCALHOST;
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
		case 'p':
			opt_port = atoi(optarg);
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

	if (opt_unix && opt_sk_msg) {
		fprintf(stderr, "SK_MSG acceleration can only be enabled with "
			"TCP sockets\n");
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

static void do_client_rr(int s)
{
	ssize_t size;
#ifdef ADDITIONAL_STATS
	struct timespec start, stop;
#endif

	char msg[MAX_MSG_SIZE];
	if (opt_http)
		strcpy(msg, http_req);

	do {
		STORE_TIME(start);
		size = send(s, msg, opt_size, 0);
		STORE_TIME(stop);
	} while (size < 0 && opt_busy_poll && errno == EAGAIN);
	if (size != opt_size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		ERR_CLOSE(s);
	}

#ifdef ADDITIONAL_STATS
	if (++iterations_count > opt_warmup) {
		send_time += (stop.tv_sec - start.tv_sec) * 1000000000
			     + stop.tv_nsec - start.tv_nsec;
	}
#endif

	unsigned rsize = 0;
	do {
		do {
			STORE_TIME(start);
			size = recv(s, msg + rsize, sizeof(msg) - rsize, 0);
			STORE_TIME(stop);
		} while (size < 0 && opt_busy_poll && errno == EAGAIN);
		if (size > 0)
			rsize += size;
	} while (!opt_http && rsize < opt_size && size > 0);
	if (!opt_http && rsize != opt_size) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}

#ifdef ADDITIONAL_STATS
	if (iterations_count > opt_warmup) {
		recv_time += (stop.tv_sec - start.tv_sec) * 1000000000
			     + stop.tv_nsec - start.tv_nsec;
	}
#endif
}

static void client(int s)
{
	printf("I'm the client\n");

	struct sockaddr *addr;
	struct sockaddr_in addr_in = {0};
	struct sockaddr_un addr_un = {0};
	int len;
	if (opt_unix) {
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, SOCKET_PATH);
		addr = (struct sockaddr *)&addr_un;
		len = sizeof(struct sockaddr_un);
	} else {
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = opt_server_addr;
		addr_in.sin_port = htons(opt_port);
		addr = (struct sockaddr *)&addr_in;
		len = sizeof(struct sockaddr_in);
	}

	if (connect(s, addr, len)) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}
	printf("Socket connected\n");

	if (opt_sk_msg) {
		int sockmap_fd = bpf_obj_get(SOCKMAP_PATH);
		if (sockmap_fd < 0) {
			fprintf(stderr, "Error retrieving sockmap: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		if (getsockname(s, (struct sockaddr *)&addr_in, &len)) {
			fprintf(stderr, "Error retrieving local address: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
		struct conn_id cid = {
			.raddr = addr_in.sin_addr.s_addr,
			.laddr = opt_server_addr,
			.rport = addr_in.sin_port,
			/* lport in host byte order for some reason */
			.lport = opt_port,
		};

		if (bpf_map_update_elem(sockmap_fd, &cid, &s, BPF_ANY)) {
			fprintf(stderr, "Error adding socket to sockmap: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		printf("Socket added to sockmap\n");

		sleep(1); /* Make sure server added his entry in the sockmap */
	}

	if (opt_busy_poll) {
		int val = 1;
		if (ioctl(s, FIONBIO, &val)) {
			fprintf(stderr, "Error setting nonblocking mode: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
	}

	if (opt_http) {
		opt_size = sizeof(http_req) - 1;
	}

	if (opt_warmup) {
		printf("Performing %u warmup RRs...\n", opt_warmup);
		for (unsigned long i = 0; i < opt_warmup; i++)
			do_client_rr(s);
	}

	if (opt_http) {
		printf("Sending %u HTTP requests with %u ms of delay\n",
		       opt_iterations, opt_delay);
	} else {
		printf("Sending %u requests of %u bytes with %u ms of delay\n",
		       opt_iterations, opt_size, opt_delay);
	}

	unsigned long total = 0, latency;
	struct timespec start = {0}, stop;

	if (!opt_delay)
		clock_gettime(CLOCK_MONOTONIC, &start);

	for (unsigned long i = 0; i < opt_iterations; i++) {
		if (opt_delay) {
			usleep(opt_delay * 1000);
			clock_gettime(CLOCK_MONOTONIC, &start);
		}

		do_client_rr(s);

		if (opt_delay) {
			clock_gettime(CLOCK_MONOTONIC, &stop);
			latency = (stop.tv_sec - start.tv_sec) * 1000000000
				  + stop.tv_nsec - start.tv_nsec;
			total += latency;
			printf("%lu=%lu\n", i, latency);
		}
	}

	if (!opt_delay) {
		clock_gettime(CLOCK_MONOTONIC, &stop);
		total = (stop.tv_sec - start.tv_sec) * 1000000000
			+ stop.tv_nsec - start.tv_nsec;
	}

	close(s);
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

static void server(int s, char *path)
{
	struct bpf_object *bpf_prog;
	int bpf_prog_fd, sockmap_fd;
	char msg[MAX_MSG_SIZE];

	printf("I'm the server\n");

	if (opt_sk_msg) {
		char prog_path[PATH_MAX];
		strcpy(prog_path, dirname(path));
		strcat(prog_path, "/redirect.bpf.o");
		if (bpf_prog_load(prog_path, BPF_PROG_TYPE_SK_MSG, &bpf_prog,
				  &bpf_prog_fd)) {
			fprintf(stderr, "Error loading eBPF program: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
		printf("eBPF program loaded\n");

		sockmap_fd = bpf_object__find_map_fd_by_name(bpf_prog,
							     "sockmap");
		if (sockmap_fd < 0) {
			fprintf(stderr, "Error retrieving sockmap: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		if (bpf_prog_attach(bpf_prog_fd, sockmap_fd, BPF_SK_MSG_VERDICT,
				    0)) {
			fprintf(stderr, "Error attaching eBPF program: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		/* Unpin in case there are leftovers from a previous run */
		unlink(SOCKMAP_PATH);
		if (bpf_obj_pin(sockmap_fd, SOCKMAP_PATH)) {
			fprintf(stderr, "Error pinning sockmap: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
		printf("Sockmap pinned\n");
	}

	int v = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v))) {
		fprintf(stderr, "Unable to set SO_REUSEPORT sockopt\n");
		ERR_UNPIN(s);
	}

	struct sockaddr *addr;
	struct sockaddr_in addr_in = {0};
	struct sockaddr_un addr_un = {0};
	int len;
	if (opt_unix) {
		unlink(SOCKET_PATH);
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, SOCKET_PATH);
		addr = (struct sockaddr *)&addr_un;
		len = sizeof(struct sockaddr_un);
	} else {
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = INADDR_ANY; /* hotnl() */
		addr_in.sin_port = htons(opt_port);
		addr = (struct sockaddr *)&addr_in;
		len = sizeof(struct sockaddr_in);
	}

	if (bind(s, addr, len)) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		ERR_UNPIN(s);
	}
	printf("Socket bound\n");

	if (listen(s, 8)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		ERR_UNPIN(s);
	}
	printf("Socket listening\n");

	int cs;
	cs = accept(s, addr, &len);
	if (cs < 0) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(errno));
		ERR_UNPIN(s);
	}
	printf("Connection accepted\n");

	close(s);
	if (opt_unix)
		unlink(SOCKET_PATH);
	s = cs;
	printf("Listening socket closed\n");

	if (opt_busy_poll) {
		int val = 1;
		if (ioctl(s, FIONBIO, &val)) {
			fprintf(stderr, "Error setting nonblocking mode: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
	}

	if (opt_sk_msg) {
		struct conn_id cid = {
			.raddr = opt_server_addr,
			.laddr = addr_in.sin_addr.s_addr,
			.rport = htons(opt_port),
			/* lport in host byte order for some reason */
			.lport = ntohs(addr_in.sin_port),
		};

		if (bpf_map_update_elem(sockmap_fd, &cid, &s, BPF_ANY)) {
			fprintf(stderr, "Error adding socket to sockmap: %s\n",
				strerror(errno));
			ERR_UNPIN(s);
		}

		printf("Socket added to sockmap\n");
	}

	if (opt_http) {
		printf("Handling HTTP requests with %u B replies\n",
		       opt_size);
	} else {
		printf("Handling requests\n");
	}
	
	/* Handle requests until the connection is closed by the client */
	for (;;) {
#ifdef ADDITIONAL_STATS
		struct timespec start, stop;
#endif
		ssize_t rsize = 0, ssize, rc;

		do {
			do {
				STORE_TIME(start);
				rc = recv(s, msg + rsize, sizeof(msg) - rsize,
					  0);
				STORE_TIME(stop);
			} while (rc < 0 && opt_busy_poll && errno == EAGAIN);
			if (rc > 0)
				rsize += rc;
		} while (!opt_http && rsize < opt_size && rc > 0);
		if (rc == 0) {
			break;
		} else if (rc < 0) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			ERR_UNPIN(s);
		} else if (!opt_http && rsize != opt_size) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

#ifdef ADDITIONAL_STATS
		recv_time += (stop.tv_sec - start.tv_sec) * 1000000000
			     + stop.tv_nsec - start.tv_nsec;
#endif

		if (opt_http) {
			sprintf(msg, http_resp, http_body_size);
			rsize = opt_size;
		}

		do {
			STORE_TIME(start);
			ssize = send(s, msg, rsize, 0);
			STORE_TIME(stop);
		} while (ssize < 0 && opt_busy_poll && errno == EAGAIN);
		if (ssize != rsize) {
			fprintf(stderr, "Error sending message: %s\n",
				strerror(errno));
			ERR_UNPIN(s);
		}

#ifdef ADDITIONAL_STATS
		send_time += (stop.tv_sec - start.tv_sec) * 1000000000
			     + stop.tv_nsec - start.tv_nsec;
		iterations_count++;
#endif
	}

	printf("Test terminated\n");

	close(s);
	printf("Socket closed\n");

	if (opt_sk_msg) {
		if (unlink(SOCKMAP_PATH))
			fprintf(stderr, "Error unpinning sockmap: %s\n",
				strerror(errno));
		else
			printf("Sockmap unpinned\n");
	}

#ifdef ADDITIONAL_STATS
	printf("Average send time %lu ns\n", send_time / iterations_count);
	printf("Average recv time %lu ns\n", recv_time / iterations_count);
#endif
}

int main(int argc, char *argv[])
{
	int s;

	parse_command_line(argc, argv);

	s = socket(opt_unix ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 1;
	}
	printf("Socket created\n");

	if (opt_client)
		client(s);
	else
		server(s, argv[0]);

	return 0;
}