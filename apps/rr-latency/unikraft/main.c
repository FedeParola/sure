#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <uk/plat/time.h>
#include <unistd.h>

#define DEFAULT_SIZE 64
#define DEFAULT_WARMUP 0
#define DEFAULT_DELAY 0
#define SERVER_IP 0x0100000a /* 10.0.0.1, already in nbo */
#define SERVER_PORT 5000
#define MAX_MSG_SIZE 16384
#define ERR_CLOSE(s) ({ close(s); exit(1); })

static unsigned opt_iterations = 0;
static unsigned opt_size = DEFAULT_SIZE;
static int opt_client = 0;
static unsigned opt_warmup = DEFAULT_WARMUP;
static unsigned opt_delay = DEFAULT_DELAY;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
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
		"  -w, --warmup		Number of warmup iterations (default %u)\n"
		"  -d, --delay		Delay between consecutive requests in ms (default %u)\n",
		prog, DEFAULT_SIZE, DEFAULT_WARMUP, DEFAULT_DELAY);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cw:d:", long_options,
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

static void do_client_rr(int s)
{
	char msg[MAX_MSG_SIZE];

	if (send(s, msg, opt_size, 0) != opt_size) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		ERR_CLOSE(s);
	}

	unsigned rsize = 0, size;
	do {
		size = recv(s, msg + rsize, sizeof(msg) - rsize, 0);
		if (size > 0)
			rsize += size;
	} while (rsize < opt_size && size > 0);
	if (rsize != opt_size) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}
}

static void client(int s)
{
	printf("I'm the client\n");

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = SERVER_IP;
	addr.sin_port = htons(SERVER_PORT);
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error connecting to server: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}
	printf("Socket connected\n");

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

	printf("total-time=%lu\nrr-latency=%lu\n", total,
	       total / opt_iterations);

	close(s);
	printf("Socket closed\n");
}

static void server(int s)
{
	char msg[MAX_MSG_SIZE];

	printf("I'm the server\n");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; /* hotnl() */
	addr.sin_port = htons(SERVER_PORT);
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		ERR_CLOSE(s);
	}
	printf("Socket bound\n");

	if (listen(s, 8)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		ERR_CLOSE(s);
	}
	printf("Socket listening\n");

	int cs;
	cs = accept(s, NULL, NULL);
	if (cs < 0) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(errno));
		ERR_CLOSE(s);
	}
	printf("Connection accepted\n");

	close(s);
	printf("Listening socket closed\n");

	s = cs;

	printf("Handling requests\n");
	
	/* Handle requests until the connection is closed by the client */
	for (;;) {
		unsigned rsize = 0, size;
		do {
			size = recv(s, msg + rsize, sizeof(msg) - rsize, 0);
			if (size > 0)
				rsize += size;
		} while (rsize < opt_size && size > 0);
		if (size == 0) {
			break;
		} else if (rsize != opt_size) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}

		if (send(s, msg, rsize, 0) != rsize) {
			fprintf(stderr, "Error sending message: %s\n",
				strerror(errno));
			ERR_CLOSE(s);
		}
	}

	printf("Test terminated\n");

	close(s);
	printf("Socket closed\n");
}

int main(int argc, char *argv[])
{
	int s;

	parse_command_line(argc, argv);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 1;
	}
	printf("Socket created\n");

	if (opt_client)
		client(s);
	else
		server(s);

	return 0;
}