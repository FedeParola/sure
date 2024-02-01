/*
 * Some sort of Copyright
 */

#include "../common/service/service_sync.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static void SendOrderConfirmation(SendOrderConfirmationRR *rr __unused) {
	DEBUG("A request to send order confirmation email to %s has been received\n", rr->req.Email);
	return;
}

static void handle_request(struct unimsg_shm_desc *desc)
{
	struct rpc *rpc = desc->addr;
	SendOrderConfirmationRR *rr = (SendOrderConfirmationRR *)rpc->rr;

	SendOrderConfirmation(rr);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(EMAIL_SERVICE, handle_request);

	return 0;
}
