/*
 * Some sort of Copyright
 */

#include <math.h>
// #include <uuid.h>
#include "../common/service/service_sync.h"

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
	DEBUG("[Charge] received request\n");
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
		DEBUG("Credit card info is invalid\n");
		return;
	}

	// Only VISA and mastercard is accepted,
	// other card types (AMEX, dinersclub) will
	// throw UnacceptedCreditCard error.
	if ((strcmp(cardType, "visa") != 0) && (strcmp(cardType, "mastercard") != 0)) {
		DEBUG("Sorry, we cannot process %s credit cards. Only VISA or MasterCard is accepted.\n", cardType);
		return;
	}

	// Also validate expiration is > today.
	int32_t currentMonth = 5;
	int32_t currentYear = 2022;
	int32_t year = in->CreditCard.CreditCardExpirationYear;
	int32_t month = in->CreditCard.CreditCardExpirationMonth;
	if ((currentYear * 12 + currentMonth) > (year * 12 + month)) { // throw ExpiredCreditCard
		DEBUG("Your credit card (ending %s) expired on %d/%d\n", cardNumber, month, year);
		return;
	}

	DEBUG("Transaction processed: %s ending %s Amount: %s%ld.%d\n", cardType, cardNumber, amount->CurrencyCode, amount->Units, amount->Nanos);
	// uuid_t binuuid;
	// uuid_generate_random(binuuid);
	// uuid_unparse(binuuid, rr->res.TransactionId);

	/* TODO: Using a constant UUID for now since musl doesn't provide uuid
	 * functions
	 */
	memcpy(rr->res.TransactionId, DEFAULT_UUID, sizeof(DEFAULT_UUID));

	return;
}

static void handle_request(struct unimsg_shm_desc *desc)
{
	struct rpc *rpc = desc->addr;
	ChargeRR *rr = (ChargeRR *)rpc->rr;

	Charge(rr);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(PAYMENT_SERVICE, handle_request);

	return 0;
}
