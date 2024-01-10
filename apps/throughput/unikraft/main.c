#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
// #include <libgen.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <uk/plat/time.h>
#include <unistd.h>

#define DEFAULT_SIZE 64
#define SERVER_ADDR 0x0100000a /* Already in nbo */
#define DEFAULT_PORT 5000
#define MAX_MSG_SIZE 16384
#define MAX_NSOCKS 256
#define ERR_CLOSE(s) ({ close(s); exit(1); })

static unsigned opt_duration;
static unsigned opt_size = DEFAULT_SIZE;
static unsigned opt_connections;
static unsigned opt_http = 0;
static unsigned http_body_size;
static uint16_t opt_port = DEFAULT_PORT;
static struct option long_options[] = {
	{"duration", required_argument, 0, 'd'},
	{"size", required_argument, 0, 's'},
	{"connections", required_argument, 0, 'c'},
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
		"  -h, --http		Use HTTP payloads\n"
		"  -p, --port		Port to listen on / connect to (default %u)\n",
		prog, DEFAULT_SIZE, DEFAULT_PORT);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "d:s:c:hp:", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			opt_duration = atoi(optarg);
			break;
		case 's':
			opt_size = atoi(optarg);
			break;
		case 'c':
			opt_connections = atoi(optarg);
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

static void client_recv(int s)
{
	ssize_t size;
	char msg[MAX_MSG_SIZE];

	size = recv(s, msg, sizeof(msg), 0);
	if (size <= 0 || (!opt_http && size != opt_size)) {
		fprintf(stderr, "Error receiving message: %s\n",
			strerror(errno));
		exit(1);
	}
}

static void client()
{
	struct pollfd pollfds[MAX_NSOCKS];

	printf("I'm the client\n");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = SERVER_ADDR;
	addr.sin_port = htons(opt_port);

	for (unsigned i = 0; i < opt_connections; i++) {
		int s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			fprintf(stderr, "Error creating socket: %s\n",
				strerror(errno));
			exit(1);
		}

		if (connect(s, (struct sockaddr *)&addr, sizeof(addr))
		    && errno != EINPROGRESS) {
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

	if (opt_http)
		opt_size = sizeof(http_req) - 1;

	printf("Running %u connections for %u seconds with %u bytes of "
	       "message\n", opt_connections, opt_duration, opt_size);

	unsigned long start = ukplat_monotonic_clock();

	for (unsigned i = 0; i < opt_connections; i++)
		client_send(pollfds[i].fd);

	do {
		if (poll(pollfds, opt_connections, -1) <= 0) {
			fprintf(stderr, "Error polling: %s\n", strerror(errno));
			exit(1);
		}

		for (unsigned i = 0; i < opt_connections; i++) {
			if (pollfds[i].revents & POLLIN) {
				client_recv(pollfds[i].fd);
				client_send(pollfds[i].fd);

			} else if (pollfds[i].revents) {
				fprintf(stderr, "Unexpected event on socket\n");
				exit(1);
			}
		}
	} while (ukplat_monotonic_clock() - start
		 < (unsigned long)opt_duration * 1000000000);

	/* For some reason busy polling on the recv doesn't work */
	unsigned completed = 0;
	while (completed < opt_connections) {
		if (poll(pollfds, opt_connections, -1) <= 0) {
			fprintf(stderr, "Error polling: %s\n", strerror(errno));
			exit(1);
		}

		for (unsigned i = 0; i < opt_connections; i++) {
			if (pollfds[i].revents & POLLIN) {
				client_recv(pollfds[i].fd);
				close(pollfds[i].fd);
				pollfds[i].fd = -1;
				completed++;

			} else if (pollfds[i].revents) {
				fprintf(stderr, "Unexpected event on socket\n");
				exit(1);
			}
		}
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

static void server()
{
	struct pollfd pollfds[MAX_NSOCKS];
	unsigned nsocks = 1;

	printf("I'm the server\n");

	pollfds[0].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (pollfds[0].fd < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}
	pollfds[0].events = POLLIN;
	printf("Socket created\n");

	int val = 1;
	if (ioctl(pollfds[0].fd, FIONBIO, &val)) {
		fprintf(stderr, "Error setting nonblocking mode: %s\n",
			strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; /* hotnl() */
	addr.sin_port = htons(opt_port);
	if (bind(pollfds[0].fd, (struct sockaddr *)&addr, sizeof(addr))) {
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
	unsigned long start = 0;

	do {
		if (poll(pollfds, nsocks, -1) <= 0) {
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
			int s = accept(pollfds[0].fd, NULL, NULL);
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
				start = ukplat_monotonic_clock();
			}

			pollfds[nsocks].fd = s;
			pollfds[nsocks++].events = POLLIN;

		} else if (pollfds[0].revents) {
			fprintf(stderr, "Unexpected event on socket\n");
			exit(1);
		}
	} while (nsocks > 1);

	close(pollfds[0].fd);

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