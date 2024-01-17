// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <c_lib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
#include "../common/services.h"
#include "../common/messages.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

char *currencies[] = {"EUR", "USD", "JPY", "CAD"};
double conversion_rate[] = {1.0, 1.1305, 126.40, 1.5128};

static int compare_e(void* left, void* right ) {
    return strcmp((const char *)left, (const char *)right);
}

struct clib_map* currency_data_map;

static void getCurrencyData(struct clib_map* map) {
    int size = sizeof(currencies)/sizeof(currencies[0]);
    int i = 0;
    for (i = 0; i < size; i++ ) {
        char *key = clib_strdup(currencies[i]);
        int key_length = (int)strlen(key) + 1;
        double value = conversion_rate[i];
		printf("Inserting [%s -> %f]\n", key, value);
        insert_c_map(map, key, key_length, &value, sizeof(double)); 
        free(key);
    }
}

static void GetSupportedCurrencies(GetSupportedCurrenciesResponse *res){
	printf("[GetSupportedCurrencies] received request\n");

	res->num_currencies = 0;
	int size = sizeof(currencies) / sizeof(currencies[0]);
	int i = 0;
	for (i = 0; i < size; i++) {
		res->num_currencies++;
		strcpy(res->CurrencyCodes[i], currencies[i]);
	}

	// printf("[GetSupportedCurrencies] completed request\n");
	return;
}

// static void PrintSupportedCurrencies (struct http_transaction *in) {
// 	printf("Supported Currencies: ");
// 	int i = 0;
// 	for (i = 0; i < in->get_supported_currencies_response.num_currencies; i++) {
// 		printf("%s\t", in->get_supported_currencies_response.CurrencyCodes[i]);
// 	}
// 	printf("\n");
// }

/**
 * Helper function that handles decimal/fractional carrying
 */
static void Carry(Money* amount) {
	double fractionSize = pow(10, 9);
	amount->Nanos = amount->Nanos + (int32_t)((double)(amount->Units % 1) * fractionSize);
	amount->Units = (int64_t)(floor((double)amount->Units) + floor((double)amount->Nanos / fractionSize));
	amount->Nanos = amount->Nanos % (int32_t)fractionSize;
	return;
}

static void Convert(CurrencyConversionRR *rr) {
	printf("[Convert] received request\n");
	CurrencyConversionRequest* in = &rr->req;
	Money* euros = &rr->res;

	// Convert: from_currency --> EUR
	void* data;
	find_c_map(currency_data_map, in->From.CurrencyCode, &data);
	euros->Units = (int64_t)((double)in->From.Units/ *(double*)data);
	euros->Nanos = (int32_t)((double)in->From.Nanos/ *(double*)data);

	Carry(euros);
	euros->Nanos = (int32_t)(round((double) euros->Nanos));

	// Convert: EUR --> to_currency
	find_c_map(currency_data_map, in->ToCode, &data);
	euros->Units = (int64_t)((double)euros->Units/ *(double*)data);
	euros->Nanos = (int32_t)((double)euros->Nanos/ *(double*)data);
	Carry(euros);

	euros->Units = (int64_t)(floor((double)(euros->Units)));
	euros->Nanos = (int32_t)(floor((double)(euros->Nanos)));
	strcpy(euros->CurrencyCode, in->ToCode);

	printf("[Convert] completed request\n");
	return;
}

int main(int argc, char **argv)
{
	int rc;
	struct unimsg_sock *s;

	(void)argc;
	(void)argv;

	currency_data_map = new_c_map(compare_e, NULL, NULL);
	getCurrencyData(currency_data_map);

	rc = unimsg_socket(&s);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		return 1;
	}

	rc = unimsg_bind(s, services[CURRENCY_SERVICE].port);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n",
			services[CURRENCY_SERVICE].port, strerror(-rc));
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
		CurrencyRpc *rpc;

		nrecv = 1;
		rc = unimsg_recv(cs, &desc, &nrecv, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		rpc = desc.addr;

		switch (rpc->command) {
		case CURRENCY_COMMAND_GET_SUPPORTED_CURRENCIES:
			GetSupportedCurrencies(
				(GetSupportedCurrenciesResponse *)&rpc->rr);
			break;
		case CURRENCY_COMMAND_CONVERT:
			Convert((CurrencyConversionRR *)&rpc->rr);
			break;
		default:
			fprintf(stderr, "Received unknown command\n");
		}

		rc = unimsg_send(cs, &desc, 1, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			ERR_PUT(&desc, 1, s);
		}
	}
	
	return 0;
}
