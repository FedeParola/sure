#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unimsg/net.h>

#define PORT 5000
#define ITERATIONS 20

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
	int rc;
	struct unimsg_sock *lsock, *sock;

	rc = unimsg_socket(&lsock);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
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

	for (int i = 0; i < ITERATIONS; i++) {
		rc = unimsg_accept(lsock, &sock, 0);
		if (rc) {
			fprintf(stderr, "Error accepting connection: %s\n",
				strerror(-rc));
			exit(1);
		}

		struct unimsg_shm_desc desc;
		rc = unimsg_recv(sock, &desc, 0);
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
		rc = unimsg_buffer_get(&resp);
		if (rc) {
			fprintf(stderr, "Error getting buffer: %s\n",
				strerror(-rc));
			exit(1);
		}

		sprintf((char *)resp.addr, resp_template, timestr, strlen(body),
			body);
		resp.size = strlen((char *)desc.addr);

		unimsg_buffer_put(&desc);

		rc = unimsg_send(sock, &resp, 0);
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
