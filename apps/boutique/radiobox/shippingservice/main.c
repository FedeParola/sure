// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <math.h>
#include "../common/service.h"
#include "../common/messages.h"

#define DEFAULT_UUID "1b4e28ba-2fa1-11d2-883f-0016d3cca427"
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

// Quote represents a currency value.
typedef struct _quote {
	uint32_t Dollars;
	uint32_t Cents;
} Quote;

// CreateQuoteFromFloat takes a price represented as a float and creates a Price struct.
static Quote CreateQuoteFromFloat(double value) {
	double fraction, units;
	fraction = modf(value, &units);

	Quote q = {
		.Dollars = (uint32_t) units,
		.Cents = (uint32_t) trunc(fraction * 100)
	};
	return q;
}

// quoteByCountFloat takes a number of items and generates a price quote represented as a float.
static double quoteByCountFloat(int count) {
	if (count == 0) {
		return 0;
	}
	return 8.99;
}

// CreateQuoteFromCount takes a number of items and returns a Price struct.
static Quote CreateQuoteFromCount(int count) {
	return CreateQuoteFromFloat(quoteByCountFloat(count));
}

// GetQuote produces a shipping quote (cost) in USD.
static void GetQuote(GetQuoteRR *rr) {
	printf("[GetQuote] received request\n");
	
	GetQuoteRequest* in = &rr->req;

	// 1. Our quote system requires the total number of items to be shipped.
	int count = 0;
	int i;
	// printf("num_items: %d\n", in->num_items);
	for (i = 0; i < in->num_items; i++) {
		count += in->Items[i].Quantity;
	}

	// 2. Generate a quote based on the total number of items to be shipped.
	Quote quote = CreateQuoteFromCount(count);

	// 3. Generate a response.
	GetQuoteResponse* out = &rr->res;
	strcpy(out->CostUsd.CurrencyCode, "USD");
	out->CostUsd.Units = (int64_t) quote.Dollars;
	out->CostUsd.Nanos = (int32_t) (quote.Cents * 10000000);
	
	return;
}

static void MockGetQuoteRequest(GetQuoteRR *rr) {
	GetQuoteRequest* in = &rr->req;
	in->num_items = 0;

	int i;
	for (i = 0; i < 3; i++) {
		in->Items[i].Quantity = i + 1;
		in->num_items++;
	}

	return;
}

// getRandomLetterCode generates a code point value for a capital letter.
// static uint32_t getRandomLetterCode() {
// 	return 65 + (uint32_t) (rand() % 25);
// }

// getRandomNumber generates a string representation of a number with the requested number of digits.
// static void getRandomNumber(int digits, char *str) {
// 	char tmp[40];
// 	int i;
// 	for (i = 0; i < digits; i++) {
// 		sprintf(tmp, "%d", rand() % 10);
// 		strcat(str, tmp);
// 	}

// 	return;
// }

// CreateTrackingId generates a tracking ID.
static void CreateTrackingId(char *salt, char* out) {
	// char random_n_1[40]; getRandomNumber(3, random_n_1);
	// char random_n_2[40]; getRandomNumber(7, random_n_2);

	// // Use UUID instead of generating a tracking ID
	// uuid_t binuuid; uuid_generate_random(binuuid);
	// uuid_unparse(binuuid, out);

	/* TODO: Using a constant UUID for now since musl doesn't provide uuid
	 * functions
	 */
	memcpy(out, DEFAULT_UUID, sizeof(DEFAULT_UUID));

	// 2. Generate a response.
	// sprintf(out, "%u%u-%ld%s-%ld%s",
	// // printf("%s%s-%ld%s-%ld%s",
	// 	getRandomLetterCode(),
	// 	getRandomLetterCode(),
	// 	strlen(salt),
	// 	random_n_1,
	// 	strlen(salt)/2,
	// 	random_n_2
	// );

	return;
}

// ShipOrder mocks that the requested items will be shipped.
// It supplies a tracking ID for notional lookup of shipment delivery status.
static void ShipOrder(ShipOrderRR *rr) {
	printf("[ShipOrder] received request\n");
	ShipOrderRequest *in = &rr->req;
	
	// 1. Create a Tracking ID
	char baseAddress[100] = "";
	strcat(baseAddress, in->address.StreetAddress);
	strcat(baseAddress, ", ");
	strcat(baseAddress, in->address.City);
	strcat(baseAddress, ", ");
	strcat(baseAddress, in->address.State);

	ShipOrderResponse *out = &rr->res;
	CreateTrackingId(baseAddress, out->TrackingId);

	return;
}

static void MockShipOrderRequest(ShipOrderRR *rr) {
	ShipOrderRequest *in = &rr->req;
	strcpy(in->address.StreetAddress, "1600 Amphitheatre Parkway");
	strcpy(in->address.City, "Mountain View");
	strcpy(in->address.State, "CA");
	strcpy(in->address.Country, "United States");
	in->address.ZipCode = 94043;
}

static void handle_request(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned nrecv;
	ShippingRpc *rpc;

	nrecv = 1;
	int rc = unimsg_recv(s, &desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	rpc = desc.addr;

	switch (rpc->command) {
	case SHIPPING_COMMAND_GET_QUOTE:
		GetQuote((GetQuoteRR *)&rpc->rr);
		break;
	case SHIPPING_COMMAND_SHIP_ORDER:
		ShipOrder((ShipOrderRR *)&rpc->rr);
		break;
	default:
		fprintf(stderr, "Received unknown command\n");
	}

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

	run_service(SHIPPING_SERVICE, handle_request);
	
	return 0;
}

