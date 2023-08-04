#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define PORT 5000
#define ITERATIONS 20
#define MAX_MSG_SIZE 4096

static char expected_req[] = "POST /api/echo HTTP/1.1\r\n";
static char resp_template[] = "HTTP/1.1 200 OK\r\n"
			      "Server: Custom/1.0.0 Custom/1.0.0\r\n"
			      "Date: %s\r\n" /* current datetime */
			      "Content-Length: %lu\r\n" /* body length */
			      /* TODO:  what to put here? */
			      "Connection: close\r\n"
			      "\r\n"
			      "%s"; /* body */

int main()
{
	int lsock, sock;
	char req[MAX_MSG_SIZE], resp[MAX_MSG_SIZE];

	lsock = socket(AF_INET, SOCK_STREAM, 0);
	if (lsock < 0) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0x7f000001);
	addr.sin_port = htons(PORT);
	if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr))) {
		fprintf(stderr, "Error binding: %s\n", strerror(errno));
		exit(1);
	}

	if (listen(lsock, 10)) {
		fprintf(stderr, "Error listening: %s\n", strerror(errno));
		exit(1);
	}

	printf("[RC] Waiting for TS connections\n");

	for (int i = 0; i < ITERATIONS; i++) {
		sock = accept(lsock, NULL, NULL);
		if (sock < 0) {
			fprintf(stderr, "Error accepting connection: %s\n",
				strerror(errno));
			exit(1);
		}

		ssize_t size = recv(sock, req, sizeof(req), 0);
		if (size <= 0) {
			fprintf(stderr, "Error receiving message: %s\n",
				strerror(errno));
			exit(1);
		}
		req[size] = 0;

		/* Check the header is good */
		if (strncmp(req, expected_req, sizeof(expected_req) - 1)) {
			fprintf(stderr, "Received unexpected message:\n%s\n",
				req);
			exit(1);
		}

		/* Locate payload */
		char *body = strstr(req, "\r\n\r\n");
		if (!body) {
			fprintf(stderr, "Received request without payload:\n%s",
				req);
			exit(1);
		}
		body += 4; /* Skip newlines */

		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char timestr[64];
		if (!strftime(timestr, sizeof(timestr), "%c", tm)) {
			fprintf(stderr, "Error getting time string: %s\n",
				strerror(errno));
			exit(1);
		}

		sprintf(resp, resp_template, timestr, strlen(body), body);

		if (send(sock, resp, strlen(resp), 0) != strlen(resp)) {
			fprintf(stderr, "Error sending message: %s\n",
				strerror(errno));
			exit(1);
		}

		close(sock);
	}

	close(lsock);

	return 0;
}
