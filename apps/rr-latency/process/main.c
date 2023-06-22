#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

// #define SERVER_IP 0x01010a0a /* Already in nbo */
// #define SERVER_IP 0x0100000a /* Already in nbo */
#define SERVER_IP 0x0100007f /* localhost, already in nbo */
#define SERVER_PORT 5000
#define MAX_MSG_SIZE 4096

static unsigned opt_iterations = 0;
static unsigned opt_size = 0;
static int opt_client = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{"size", required_argument, 0, 's'},
	{"client", optional_argument, 0, 'c'},
	{0, 0, 0, 0}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of requests-responses to exchange\n"
		"  -s, --size		Size of the message in bytes\n"
		"  -c, --client		Behave as client (default is server)\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:s:c", long_options,
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

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 1;
	}
	printf("Socket created\n");

	if (opt_client) {
		printf("I'm the client\n");

		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
    		addr.sin_addr.s_addr = SERVER_IP;
    		addr.sin_port = htons(SERVER_PORT);

		if (connect(s, (struct sockaddr *)&addr, sizeof(addr))) {
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

		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
    		addr.sin_addr.s_addr = INADDR_ANY; /* hotnl() */
    		addr.sin_port = htons(SERVER_PORT);

		if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) {
			fprintf(stderr, "Error binding to port %d: %s\n",
				SERVER_PORT, strerror(errno));
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