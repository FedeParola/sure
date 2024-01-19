/*
 * Some sort of Copyright
 */

#include "../common/service/service.h"
#include "../common/service/message.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static void SendOrderConfirmation(SendOrderConfirmationRR *rr) {
	DEBUG("A request to send order confirmation email to %s has been received\n", rr->req.Email);
	return;
}

static void handle_request(struct unimsg_shm_desc *descs,
			   unsigned *ndescs __unused)
{
	SendOrderConfirmationRR *rr = descs[0].addr;

	SendOrderConfirmation(rr);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(EMAIL_SERVICE, handle_request);
	
	return 0;
}
