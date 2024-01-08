/*
 * Some sort of Copyright
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
// #include <uuid.h>
#include "../common/services.h"
#include "../common/messages.h"

#define DEFAULT_UUID "1b4e28ba-2fa1-11d2-883f-0016d3cca427"
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static int get_digits(int64_t num) {
	//returns the number of digits
	return (int)floor(log10(num));
}

static int get_digit_sum(int n) {
	return (int)(n / 10) + (n % 10);
}

static char* creditcard_validator(int64_t credit_card) {
	int digits = get_digits(credit_card);
	int sum = 0;
	int first_digits = 0;
	char* card_type;
	int i;
	digits++;

	for (i = 0; i < digits; i++) {
		if (i & 1)
			sum += get_digit_sum(2 * (credit_card % 10));
		else
			sum += credit_card % 10;

		if (i == digits - 2)
			first_digits = credit_card % 10;
		else if (i == digits - 1)
			first_digits = first_digits + (credit_card % 10) * 10;

		credit_card /= 10;
	}

	if (!(sum % 10)) {
		if (digits == 15
		    && (first_digits == 34 || first_digits == 37)) {
			card_type = "amex";
		} else if (digits == 16
			 && ((first_digits >= 50 && first_digits <= 55)
			     || (first_digits >= 22 && first_digits <= 27))) {
			card_type = "mastercard";
		} else if ((digits >= 13 && digits <= 16)
			   && (first_digits / 10 == 4)) {
			card_type = "visa";
		} else {
			card_type = "invalid";
		}
	} else {
		card_type = "invalid";
	}

	return card_type;
}

static void Charge(ChargeRR *rr) {
	printf("[Charge] received request\n");
	ChargeRequest* in = &rr->req;

	Money* amount = &in->Amount;
	char* cardNumber = in->CreditCard.CreditCardNumber;

	char* cardType;
	bool valid = false;
	cardType = creditcard_validator(strtoll(cardNumber, NULL, 10));
	if (strcmp(cardType, "invalid")) {
		valid = true;
	}

	if (!valid) { // throw InvalidCreditCard 
		printf("Credit card info is invalid\n");
		return;
	}

	// Only VISA and mastercard is accepted, 
	// other card types (AMEX, dinersclub) will
	// throw UnacceptedCreditCard error.
	if ((strcmp(cardType, "visa") != 0) && (strcmp(cardType, "mastercard") != 0)) {
		printf("Sorry, we cannot process %s credit cards. Only VISA or MasterCard is accepted.\n", cardType);
		return;
	}

	// Also validate expiration is > today.
	int32_t currentMonth = 5;
	int32_t currentYear = 2022;
	int32_t year = in->CreditCard.CreditCardExpirationYear;
	int32_t month = in->CreditCard.CreditCardExpirationMonth;
	if ((currentYear * 12 + currentMonth) > (year * 12 + month)) { // throw ExpiredCreditCard
		printf("Your credit card (ending %s) expired on %d/%d\n", cardNumber, month, year);
		return;
	}

	printf("Transaction processed: %s ending %s Amount: %s%ld.%d\n", cardType, cardNumber, amount->CurrencyCode, amount->Units, amount->Nanos);
	// uuid_t binuuid;
	// uuid_generate_random(binuuid);
	// uuid_unparse(binuuid, rr->res.TransactionId);
	
	/* TODO: Using a constant UUID for now since musl doesn't provide uuid
	 * functions
	 */
	memcpy(rr->res.TransactionId, DEFAULT_UUID, sizeof(DEFAULT_UUID));

	return;
}

static void MockChargeRequest(ChargeRR *rr) {
	strcpy(rr->req.CreditCard.CreditCardNumber, "4432801561520454");
	rr->req.CreditCard.CreditCardCvv = 672;
	rr->req.CreditCard.CreditCardExpirationYear = 2039;
	rr->req.CreditCard.CreditCardExpirationMonth = 1;

	strcpy(rr->req.Amount.CurrencyCode, "USD");
	rr->req.Amount.Units = 300;
	rr->req.Amount.Nanos = 2;
}

static void PrintChargeResponse(ChargeRR *rr) {
	printf("TransactionId: %s\n", rr->res.TransactionId);
}

int main(int argc, char **argv)
{
	int rc;
	struct unimsg_sock *s;

	(void)argc;
	(void)argv;

	printf("Size of message is %lu B\n", sizeof(ChargeRR));

	rc = unimsg_socket(&s);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		return 1;
	}

	rc = unimsg_bind(s, PAYMENT_PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", PAYMENT_PORT,
			strerror(-rc));
		ERR_CLOSE(s);
	}

	rc = unimsg_listen(s);
	if (rc) {
		fprintf(stderr, "Error listening: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	printf("Waiting for incoming connections...\n");

	struct unimsg_sock *cs;
	rc = unimsg_accept(s, &cs, 0);
	if (rc) {
		fprintf(stderr, "Error accepting connection: %s\n",
			strerror(-rc));
		ERR_CLOSE(s);
	}

	printf("Client connected\n");

	while (1) {
		struct unimsg_shm_desc desc;
		unsigned nrecv;
		ChargeRR *rr;

		nrecv = 1;
		rc = unimsg_recv(cs, &desc, &nrecv, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		rr = desc.addr;

		Charge(rr);

		rc = unimsg_send(cs, &desc, 1, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			ERR_PUT(&desc, 1, s);
		}
	}
	
	return 0;
}
