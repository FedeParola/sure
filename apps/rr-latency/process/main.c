#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

// #define SERVER_IP 0x01010a0a /* Already in nbo */
// #define SERVER_IP 0x0100000a /* Already in nbo */
#define SERVER_IP 0x0100007f /* localhost, already in nbo */
#define SERVER_PORT 5000
#define MAX_MSG_SIZE 4096
#define SOCKET_PATH "/tmp/rr-latency.sock"

static unsigned opt_iterations = 0;
static unsigned opt_size = 0;
static int opt_client = 0;
static int opt_unix = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{"unix", optional_argument, 0, 'u'},
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
		"  -u, --unix		Use AF_UNIX sockets\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:cu", long_options,
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

int main(int argc, char *argv[])
{
	int s;
	unsigned long msg[MAX_MSG_SIZE / sizeof(unsigned long)];

	parse_command_line(argc, argv);

	s = socket(opt_unix ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 1;
	}
	printf("Socket created\n");

	if (opt_client) {
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
			addr_in.sin_addr.s_addr = SERVER_IP;
			addr_in.sin_port = htons(SERVER_PORT);
			addr = (struct sockaddr *)&addr_in;
			len = sizeof(struct sockaddr_in);
		}

		if (connect(s, addr, len)) {
			fprintf(stderr, "Error connecting to server: %s\n",
				strerror(errno));
			goto err_close;
		}
		printf("Socket connected\n");

		printf("Sending %u requests of %u bytes\n", opt_iterations,
		       opt_size);

		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);

		for (unsigned long i = 0; i < opt_iterations; i++) {
			for (unsigned j = 0; j < opt_size / 8; j++)
				msg[j] = i;

			if (send(s, msg, opt_size, 0) != opt_size) {
				fprintf(stderr, "Error sending message: %s\n",
					strerror(errno));
				goto err_close;
			}

			if (recv(s, msg, opt_size, 0) != opt_size) {
				fprintf(stderr, "Error receiving message: %s\n",
					strerror(errno));
				goto err_close;
			}

			for (unsigned j = 0; j < opt_size / 8; j++)
				if (msg[j] != i + 1) {
					fprintf(stderr, "Received unexpected "
						"message\n");
					goto err_close;
				}
		}

		struct timespec stop;
		clock_gettime(CLOCK_MONOTONIC, &stop);
		unsigned long elapsed =
			(stop.tv_sec - start.tv_sec) * 1000000000
			+ stop.tv_nsec - start.tv_nsec;

		printf("Total time: %lu ns, average rr latency: %lu ns\n",
		       elapsed, elapsed / opt_iterations);

	} else {
		printf("I'm the server\n");

		int v = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v))) {
			fprintf(stderr, "Unable to set SO_REUSEPORT sockopt\n");
			goto err_close;
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
			goto err_close;
		}
		printf("Socket bound\n");

		if (listen(s, 8)) {
			fprintf(stderr, "Error listening: %s\n",
				strerror(errno));
			goto err_close;
		}
		printf("Socket listening\n");

		int cs;
		cs = accept(s, NULL, 0);
		if (cs < 0) {
			fprintf(stderr, "Error accepting connection: %s\n",
				strerror(errno));
			goto err_close;
		}
		printf("Connection accepted\n");

		close(s);
		if (opt_unix)
			unlink(SOCKET_PATH);
		s = cs;
		printf("Listening socket closed\n");

		printf("Handling requests\n");

		for (unsigned i = 0; i < opt_iterations; i++) {
			if (recv(s, msg, opt_size, 0) != opt_size) {
				fprintf(stderr, "Error receiving message: %s\n",
					strerror(errno));
				goto err_close;
			}

			for (unsigned j = 0; j < opt_size / 8; j++)
				msg[j]++;

			if (send(s, msg, opt_size, 0) != opt_size) {
				fprintf(stderr, "Error sending message: %s\n",
					strerror(errno));
				goto err_close;
			}
		}

		printf("Test terminated\n");
	}

	close(s);
	printf("Socket closed\n");

	return 0;

err_close:
	close(s);
	return 1;
}