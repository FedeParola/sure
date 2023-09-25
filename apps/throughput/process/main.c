#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
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
#define SERVER_ADDR 0x0100000a /* Already in nbo */
#define LOCALHOST 0x0100007f /* localhost, already in nbo */
#define DEFAULT_PORT 5000
#define MAX_MSG_SIZE 16384
#define MAX_NSOCKS 256
#define SOCKET_PATH "/tmp/throughput.sock"
#define SOCKMAP_PATH "/sys/fs/bpf/sockmap"
#define ERR_CLOSE(s) ({ close(s); exit(1); })
#define ERR_UNPIN(s) ({ if (opt_sk_msg) unlink(SOCKMAP_PATH); ERR_CLOSE(s); })

static unsigned opt_duration;
static unsigned opt_size = DEFAULT_SIZE;
static unsigned opt_connections;
static int opt_unix = 0;
static int opt_sk_msg = 0;
static int opt_server_addr = SERVER_ADDR;
static unsigned opt_http = 0;
static unsigned http_body_size;
static uint16_t opt_port = DEFAULT_PORT;
static volatile int stop = 0;
static struct option long_options[] = {
	{"duration", required_argument, 0, 'd'},
	{"size", required_argument, 0, 's'},
	{"connections", required_argument, 0, 'c'},
	{"unix", optional_argument, 0, 'u'},
	{"sk-msg", optional_argument, 0, 'm'},
	{"localhost", optional_argument, 0, 'l'},
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
		"  -d, --duration	Duration of the test in seconds\n"
		"  -s, --size		Size of the message in bytes (default %u)\n"
		"  -c, --connections	Number of client connections (if not specified or 0, behave as server)\n"
		"  -u, --unix		Use AF_UNIX sockets\n"
		"  -m, --sk-msg		Use SK_MSG acceleration (TCP sockets only)\n"
		"  -l, --localhost	Run test on localhost\n"
		"  -h, --http		Use HTTP payloads\n"
		"  -p, --port		Port to listen on / connect to (default %u)\n",
		prog, DEFAULT_SIZE, DEFAULT_PORT);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "d:s:c:bumlhp:", long_options,
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
		case 'u':
			opt_unix = 1;
			break;
		case 'm':
			opt_sk_msg = 1;
			break;
		case 'l':
			opt_server_addr = LOCALHOST;
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

	if (opt_connections && !opt_duration) {
		fprintf(stderr, "Client must specify duration > 0\n");
		usage(argv[0]);
	}

	if (opt_unix && opt_sk_msg) {
		fprintf(stderr, "SK_MSG acceleration can only be enabled with "
			"TCP sockets\n");
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

static void client_send(int s)
{
	ssize_t size;
	char msg[MAX_MSG_SIZE];

	if (opt_http)
		strcpy(msg, http_req);

	size = send(s, msg, opt_size, 0);
	if (size != opt_size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		exit(1);
	}
}

static void client_recv(int s, int busy_poll)
{
	ssize_t size;
	char msg[MAX_MSG_SIZE];

	do {
		size = recv(s, msg, sizeof(msg), 0);
	} while (size < 0 && busy_poll && errno == EAGAIN);
	if (size < 0 || (!opt_http && size != opt_size)) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		exit(1);
	}
}

static void client()
{
	struct pollfd pollfds[MAX_NSOCKS];

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

	for (unsigned i = 0; i < opt_connections; i++) {
		int s = socket(opt_unix ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			fprintf(stderr, "Error creating socket: %s\n",
				strerror(errno));
			exit(1);
		}

		if (connect(s, addr, len) && errno != EINPROGRESS) {
			fprintf(stderr, "Error connecting to server: %s\n",
				strerror(errno));
			exit(1);
		}

		pollfds[i].fd = s;
		pollfds[i].events = POLLIN;

		int val = 1;
		if (ioctl(s, FIONBIO, &val)) {
			fprintf(stderr, "Error setting nonblocking mode: %s\n",
				strerror(errno));
			exit(1);
		}
	}
	printf("Sockets connected\n");

	// if (opt_sk_msg) {
	// 	int sockmap_fd = bpf_obj_get(SOCKMAP_PATH);
	// 	if (sockmap_fd < 0) {
	// 		fprintf(stderr, "Error retrieving sockmap: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}

	// 	if (getsockname(s, (struct sockaddr *)&addr_in, &len)) {
	// 		fprintf(stderr, "Error retrieving local address: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}
	// 	struct conn_id cid = {
	// 		.raddr = addr_in.sin_addr.s_addr,
	// 		.laddr = opt_server_addr,
	// 		.rport = addr_in.sin_port,
	// 		/* lport in host byte order for some reason */
	// 		.lport = opt_port,
	// 	};

	// 	if (bpf_map_update_elem(sockmap_fd, &cid, &s, BPF_ANY)) {
	// 		fprintf(stderr, "Error adding socket to sockmap: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}

	// 	printf("Socket added to sockmap\n");

	// 	sleep(1); /* Make sure server added his entry in the sockmap */
	// }

	if (opt_http) {
		opt_size = sizeof(http_req) - 1;
	}

	printf("Running %u connections for %u seconds with %u bytes of "
	       "message\n", opt_connections, opt_duration, opt_size);

	struct timespec start, stop;
	unsigned long elapsed;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (unsigned i = 0; i < opt_connections; i++)
		client_send(pollfds[i].fd);

	do {
		if (poll(pollfds, opt_connections, -1) <= 0) {
			fprintf(stderr, "Error polling: %s\n", strerror(errno));
			exit(1);
		}

		for (unsigned i = 0; i < opt_connections; i++) {
			if (pollfds[i].revents & POLLIN) {
				client_recv(pollfds[i].fd, 0);
				client_send(pollfds[i].fd);

			} else if (pollfds[i].revents) {
				fprintf(stderr, "Unexpected event on socket\n");
				exit(1);
			}
		}

		clock_gettime(CLOCK_MONOTONIC, &stop);
		elapsed = (stop.tv_sec - start.tv_sec) * 1000000000
			  + stop.tv_nsec - start.tv_nsec;
	} while (elapsed < (unsigned long)opt_duration * 1000000000);

	for (unsigned i = 0; i < opt_connections; i++) {
		client_recv(pollfds[i].fd, 1);
		close(pollfds[i].fd);
	}

	printf("Sockets closed\n");
}

static int do_server_rr(int s)
{
	char msg[MAX_MSG_SIZE];

	ssize_t rsize, ssize;
	rsize = recv(s, msg, sizeof(msg), 0);
	if (rsize <= 0) {
		if (rsize == 0) {
			return 1;

		} else {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			exit(1);
		}
	}

	if (opt_http) {
		sprintf(msg, http_resp, http_body_size);
		rsize = opt_size;
	}

	ssize = send(s, msg, rsize, 0);
	if (ssize != rsize) {
		fprintf(stderr, "Error sending message: %s\n",
			strerror(errno));
		exit(1);
	}

	return 0;
}

static void server(char *path)
{
	struct bpf_object *bpf_prog;
	int bpf_prog_fd, sockmap_fd;
	struct pollfd pollfds[MAX_NSOCKS];
	unsigned nsocks = 1;

	printf("I'm the server\n");

	pollfds[0].fd = socket(opt_unix ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
	if (pollfds[0].fd < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}
	pollfds[0].events = POLLIN;
	printf("Socket created\n");

	// if (opt_sk_msg) {
	// 	char prog_path[PATH_MAX];
	// 	strcpy(prog_path, dirname(path));
	// 	strcat(prog_path, "/redirect.bpf.o");
	// 	if (bpf_prog_load(prog_path, BPF_PROG_TYPE_SK_MSG, &bpf_prog,
	// 			  &bpf_prog_fd)) {
	// 		fprintf(stderr, "Error loading eBPF program: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}
	// 	printf("eBPF program loaded\n");

	// 	sockmap_fd = bpf_object__find_map_fd_by_name(bpf_prog,
	// 						     "sockmap");
	// 	if (sockmap_fd < 0) {
	// 		fprintf(stderr, "Error retrieving sockmap: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}

	// 	if (bpf_prog_attach(bpf_prog_fd, sockmap_fd, BPF_SK_MSG_VERDICT,
	// 			    0)) {
	// 		fprintf(stderr, "Error attaching eBPF program: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}

	// 	/* Unpin in case there are leftovers from a previous run */
	// 	unlink(SOCKMAP_PATH);
	// 	if (bpf_obj_pin(sockmap_fd, SOCKMAP_PATH)) {
	// 		fprintf(stderr, "Error pinning sockmap: %s\n",
	// 			strerror(errno));
	// 		exit(1);
	// 	}
	// 	printf("Sockmap pinned\n");
	// }

	int val = 1;
	if (ioctl(pollfds[0].fd, FIONBIO, &val)) {
		fprintf(stderr, "Error setting nonblocking mode: %s\n",
			strerror(errno));
		exit(1);
	}
	if (setsockopt(pollfds[0].fd, SOL_SOCKET, SO_REUSEPORT, &val,
		       sizeof(val))) {
		fprintf(stderr, "Unable to set SO_REUSEPORT sockopt\n");
		exit(1);
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

	if (bind(pollfds[0].fd, addr, len)) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		exit(1);
	}
	printf("Socket bound\n");

	if (listen(pollfds[0].fd, 128)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		exit(1);
	}
	printf("Socket listening\n");

	unsigned long rrs = 0;
	struct timespec start, end;

	do {
		/* Server poll() can be interrupted by a singal */
		if (poll(pollfds, nsocks, -1) <= 0 && errno != EINTR) {
			fprintf(stderr, "Error polling: %s\n", strerror(errno));
			exit(1);
		}

		for (unsigned i = 1; i < nsocks; i++) {
			if (pollfds[i].revents & POLLIN) {
				if (do_server_rr(pollfds[i].fd)) {
					close(pollfds[i].fd);
					unsigned j;
					for (j = i; j < nsocks - 1; j++)
						pollfds[j] = pollfds[j + 1];
					nsocks--;
				} else {
					rrs++;
				}

			} else if (pollfds[i].revents) {
				fprintf(stderr, "Unexpected event on socket\n");
				exit(1);
			}
		}
		
		if (pollfds[0].revents && POLLIN) {
			int s = accept(pollfds[0].fd, addr, &len);
			if (s < 0) {
				fprintf(stderr, "Error accepting connection: "
					"%s\n", strerror(errno));
				exit(1);
			}

			if (ioctl(s, FIONBIO, &val)) {
				fprintf(stderr, "Error setting nonblocking "
					"mode: %s\n", strerror(errno));
				exit(1);
			}

			if (nsocks == 1) {
				printf("Handling connections\n");
				clock_gettime(CLOCK_MONOTONIC, &start);
			}

			pollfds[nsocks].fd = s;
			pollfds[nsocks++].events = POLLIN;

		} else if (pollfds[0].revents) {
			fprintf(stderr, "Unexpected event on socket\n");
			exit(1);
		}
	} while (nsocks > 1 && !stop);

	close(pollfds[0].fd);
	if (opt_unix)
		unlink(SOCKET_PATH);

	clock_gettime(CLOCK_MONOTONIC, &end);
	unsigned long elapsed = elapsed = (end.tv_sec - start.tv_sec)
					  * 1000000000
					  + end.tv_nsec - start.tv_nsec;

	printf("Sockets closed\n");

	printf("rrs=%lu\nrps=%lu\n", rrs, rrs * 1000000000 / elapsed);
}

static void sigint_handler(int signum)
{
	stop = 1;
}

int main(int argc, char *argv[])
{
	parse_command_line(argc, argv);

	if (signal(SIGINT, sigint_handler)) {
		fprintf(stderr, "Error setting signal handler: %s\n",
			strerror(errno));
		return 1;
	}

	if (opt_connections)
		client();
	else
		server(argv[0]);

	return 0;
}