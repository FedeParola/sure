#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unimsg/net.h>

// struct rest_resp {
//     long status_code;
//     char *body;
//     unimsg_shm_desc desc;
// };

// static char post_template[] = "POST %s HTTP/1.1\r\n" /* url */
// 			      "Host: %u:%u\r\n" /* addr:port */
// 			      "Accept: application/json\r\n"
// 			      "Content-Type: application/json\r\n"
// 			      "Content-Length: %u\r\n" /* body length */
// 			      "\r\n"
// 			      "%s"; /* body */

// static struct rest_resp do_post(__u32 addr, __u16 port, string url, string body)
// {
// 	struct unimsg_sock *rc_sock;
// 	struct unimsg_shm_desc desc;
// 	int rc;

// 	// int rc = unimsg_sock_create(&rc_sock, UNIMSG_SOCK_CONNECTED, 0);
// 	// if (rc) {
// 	// 	fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
// 	// 	exit(1);
// 	// }

// 	// rc = unimsg_sock_connect(rc_sock, addr, port);
// 	// if (rc) {
// 	// 	fprintf(stderr, "Error connecting to RC: %s\n", strerror(-rc));
// 	// 	exit(1);
// 	// }

// 	rc = unimsg_buffer_get(&desc);
// 	if (rc) {
// 		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
// 		exit(1);
// 	}

// 	sprintf((char *)desc.addr, post_template, url.c_str(), addr, port,
// 		body.size(), body.c_str());
// 	desc.size = strlen((char *)desc.addr);

// 	printf("Gonna send:\n%s\n", (char *)desc.addr);

// 	unimsg_buffer_put(&desc);

// 	// rc = unimsg_sock_send(rc_sock, &desc);
// 	// if (rc) {
// 	// 	fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
// 	// 	exit(1);
// 	// }

// 	// rc = unimsg_sock_recv(rc_sock, &desc);
// 	// if (rc) {
// 	// 	fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
// 	// 	exit(1);
// 	// }

// 	// unimsg_sock_close(qp_sock);

// 	struct rest_resp resp = {0};

// 	return resp;
// }

#define PORT 5000

static char expected_req[] = "POST %s HTTP/1.1\r\n";
static char resp_template[] = "HTTP/1.1 200 OK\r\n"
			      "Server: Custom/1.0.0 Custom/1.0.0\r\n"
			      "Date: %s\r\n" /* current datetime */
			      "Content-Length: 0\r\n" /* body length */
			      /* TODO:  what to put here? */
			      "Connection: close\r\n"
			      "\r\n"
			      "%s"; /* body */

int main()
{
	int rc;
	struct unimsg_sock *sock;

	rc = unimsg_sock_create(&sock, UNIMSG_SOCK_CONNECTED, 0);
	if (rc) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_bind(sock, PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PORT,
			strerror(-rc));
		exit(1);
	}

	rc = unimsg_sock_listen(sock);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		exit(1);
	}

	printf("Waiting for TS connection\n");

	struct unimsg_sock *tmp_sock;
	rc = unimsg_sock_accept(sock, &tmp_sock);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		exit(1);
	}

	printf("TS connected\n");

	unimsg_sock_close(sock);
	sock = tmp_sock;

	struct unimsg_shm_desc desc;
	rc = unimsg_sock_recv(sock, &desc);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	/* Check the header is good */
	if (!strncmp((char *)desc.addr, expected_req, strlen(expected_req))) {
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
		fprintf(stderr, "Error getting buffer: %s\n", strerror(-rc));
		exit(1);
	}

	sprintf((char *)resp.addr, resp_template, timestr, body);
	resp.size = strlen((char *)desc.addr);

	unimsg_buffer_put(&desc);

	rc = unimsg_sock_send(sock, &resp);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unimsg_sock_close(sock);

	return 0;
}
