#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unimsg/net.h>

#define PORT 5000

static unsigned opt_iterations = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{0, 0, 0, 0}
};

static char expected_req[] = "POST /api/echo HTTP/1.1\r\n";
static char resp_template[] = "HTTP/1.1 200 OK\r\n"
			      "Server: Custom/1.0.0 Custom/1.0.0\r\n"
			      "Date: %s\r\n" /* current datetime */
			      "Content-Length: %lu\r\n" /* body length */
			      /* TODO:  what to put here? */
			      "Connection: close\r\n"
			      "\r\n"
			      "%s"; /* body */


static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of control loop iterations\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (!opt_iterations) {
		fprintf(stderr, "Iterations option must be > 0\n");
		usage(argv[0]);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	int lsock, sock;

	parse_command_line(argc, argv);

	lsock = unimsg_socket();
	if (lsock < 0) {
		fprintf(stderr, "Error creating socket: %s\n",
			strerror(-lsock));
		exit(1);
	}

	rc = unimsg_bind(lsock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_listen(lsock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	printf("[RC] Waiting for TS connections\n");

	for (unsigned i = 0; i < opt_iterations; i++) {
		sock = unimsg_accept(lsock, 0);
		if (sock < 0) {
			fprintf(stderr, "Error accepting connection: %s\n",
				strerror(-sock));
			exit(1);
		}

		struct unimsg_shm_desc desc;
		unsigned ndescs = 1;
		rc = unimsg_recv(sock, &desc, &ndescs, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			exit(1);
		}
		((char *)desc.addr)[desc.size] = 0;

		/* Check the header is good */
		if (strncmp((char *)desc.addr, expected_req,
		    sizeof(expected_req) - 1)) {
			fprintf(stderr, "Received unexpected message:\n%s\n",
				(char *)desc.addr);
			exit(1);
		}

		/* Locate payload */
		char *body = strstr(desc.addr, "\r\n\r\n");
		if (!body) {
			fprintf(stderr, "Received request without payload:\n%s",
				(char *)desc.addr);
			exit(1);
		}
		body += 4; /* Skip newlines */

		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char timestr[64];
		if (!strftime(timestr, sizeof(timestr), "%c", tm)) {
			fprintf(stderr, "Error getting time string: %s\n",
				strerror(-rc));
			exit(1);
		}

		struct unimsg_shm_desc resp;
		rc = unimsg_buffer_get(&resp, 1);
		if (rc) {
			fprintf(stderr, "Error getting buffer: %s\n",
				strerror(-rc));
			exit(1);
		}

		sprintf((char *)resp.addr, resp_template, timestr, strlen(body),
			body);
		resp.size = strlen((char *)desc.addr);

		unimsg_buffer_put(&desc , 1);

		rc = unimsg_send(sock, &resp, 1, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			exit(1);
		}

		unimsg_close(sock);
	}

	unimsg_close(lsock);

	return 0;
}
