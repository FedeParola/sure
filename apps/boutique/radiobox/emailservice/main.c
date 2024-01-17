/*
 * Some sort of Copyright
 */

#include "../common/service.h"
#include "../common/messages.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static void SendOrderConfirmation(SendOrderConfirmationRR *rr) {
	printf("A request to send order confirmation email to %s has been received\n", rr->req.Email);
	return;
}

static void MockEmailRequest(SendOrderConfirmationRR *rr) {
	strcpy(rr->req.Email, "sqi009@ucr.edu");
	return;
}

static void handle_request(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned nrecv;
	SendOrderConfirmationRR *rr;

	nrecv = 1;
	int rc = unimsg_recv(s, &desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	rr = desc.addr;

	SendOrderConfirmation(rr);

	rc = unimsg_send(s, &desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		ERR_PUT(&desc, 1, s);
	}
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(EMAIL_SERVICE, handle_request);
	
	return 0;
}
