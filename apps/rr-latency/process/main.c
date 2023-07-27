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
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

#define SERVER_ADDR 0x0100000a /* Already in nbo */
#define LOCALHOST 0x0100007f /* localhost, already in nbo */
#define SERVER_PORT 5000
#define MAX_MSG_SIZE 4096
#define SOCKET_PATH "/tmp/rr-latency.sock"
#define SOCKMAP_PATH "/sys/fs/bpf/sockmap"
#define ERR_CLOSE(s) ({ close(s); exit(1); })
#define ERR_UNPIN(s) ({ if (opt_sk_msg) unlink(SOCKMAP_PATH); ERR_CLOSE(s); })

static unsigned opt_iterations = 0;
static unsigned opt_size = 0;
static int opt_client = 0;
static int opt_unix = 0;
static int opt_sk_msg = 0;
static int opt_server_addr = SERVER_ADDR;
static unsigned opt_warmup = 0;
static unsigned opt_delay = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"unix", optional_argument, 0, 'u'},
	{"sk-msg", optional_argument, 0, 'm'},
	{"localhost", optional_argument, 0, 'l'},
	{"warmup", optional_argument, 0, 'w'},
	{"delay", optional_argument, 0, 'd'},
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
		"  -u, --unix		Use AF_UNIX sockets\n"
		"  -m, --sk-msg		Use SK_MSG acceleration (TCP socket only)\n"
		"  -l, --localhost	Run test on localhost\n"
		"  -w, --warmup		Number of warmup iterations\n"
		"  -d, --delay		Delay between consecutive requests in ms (default 0)\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cumlw:d:", long_options,
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
		default:
			usage(argv[0]);
		}
	}

	if (opt_iterations == 0 || opt_size == 0) {
		fprintf(stderr, "Iterations and size are required parameteres "
			"and must be > 0\n");
		usage(argv[0]);
	}

	if (opt_unix && opt_sk_msg) {
		fprintf(stderr, "SK_MSG acceleration can only be enabled with "
			"TCP sockets\n");
		usage(argv[0]);
	}
}

static void do_client_rr(int s, unsigned long val)
{
	unsigned long msg[MAX_MSG_SIZE / sizeof(unsigned long)];

	for (unsigned j = 0; j < opt_size / 8; j++)
		msg[j] = val;

	if (send(s, msg, opt_size, 0) != opt_size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		ERR_CLOSE(s);
	}

	if (recv(s, msg, opt_size, 0) != opt_size) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}

	for (unsigned j = 0; j < opt_size / 8; j++)
		if (msg[j] != val + 1) {
			fprintf(stderr, "Received unexpected message\n");
			ERR_CLOSE(s);
		}
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
		addr_in.sin_port = htons(SERVER_PORT);
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
			.lport = SERVER_PORT,
		};

		if (bpf_map_update_elem(sockmap_fd, &cid, &s, BPF_ANY)) {
			fprintf(stderr, "Error adding socket to sockmap: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		printf("Socket added to sockmap\n");

		sleep(1); /* Make sure server added his entry in the sockmap */
	}

	if (opt_warmup) {
		printf("Performing %u warmup RRs...\n", opt_warmup);
		for (unsigned long i = 0; i < opt_warmup; i++)
			do_client_rr(s, i);
	}

	printf("Sending %u requests of %u bytes with %u ms of delay\n",
	       opt_iterations, opt_size, opt_delay);


	unsigned long total = 0, latency;
	struct timespec start = {0}, stop;

	if (!opt_delay)
		clock_gettime(CLOCK_MONOTONIC, &start);

	for (unsigned long i = 0; i < opt_iterations; i++) {
		if (opt_delay) {
			usleep(opt_delay * 1000);
			clock_gettime(CLOCK_MONOTONIC, &start);
		}

		do_client_rr(s, i);

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

	printf("total-time=%lu\nrr-latency=%lu\n", total,
	       total / opt_iterations);

	close(s);
	printf("Socket closed\n");
}

static void do_server_rr(int s)
{
	unsigned long msg[MAX_MSG_SIZE / sizeof(unsigned long)];

	if (recv(s, msg, opt_size, 0) != opt_size) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		ERR_UNPIN(s);
	}

	for (unsigned j = 0; j < opt_size / 8; j++)
		msg[j]++;

	if (send(s, msg, opt_size, 0) != opt_size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		ERR_UNPIN(s);
	}
}

static void server(int s, char *path)
{
	struct bpf_object *bpf_prog;
	int bpf_prog_fd, sockmap_fd;

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
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, SOCKET_PATH);
		addr = (struct sockaddr *)&addr_un;
		len = sizeof(struct sockaddr_un);
	} else {
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = INADDR_ANY; /* hotnl() */
		addr_in.sin_port = htons(SERVER_PORT);
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

	if (opt_sk_msg) {
		struct conn_id cid = {
			.raddr = opt_server_addr,
			.laddr = addr_in.sin_addr.s_addr,
			.rport = htons(SERVER_PORT),
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

	if (opt_warmup) {
		printf("Performing %u warmup RRs...\n", opt_warmup);
		for (unsigned long i = 0; i < opt_warmup; i++)
			do_server_rr(s);
	}

	printf("Handling requests\n");
	
	for (unsigned i = 0; i < opt_iterations; i++)
			do_server_rr(s);

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