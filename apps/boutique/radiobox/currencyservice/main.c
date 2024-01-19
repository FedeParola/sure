// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <c_lib.h>
#include <math.h>
#include "../common/service/service.h"
#include "../common/service/message.h"

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
		DEBUG("Inserting [%s -> %f]\n", key, value);
        insert_c_map(map, key, key_length, &value, sizeof(double)); 
        free(key);
    }
}

static void GetSupportedCurrencies(GetSupportedCurrenciesResponse *res){
	DEBUG("[GetSupportedCurrencies] received request\n");

	res->num_currencies = 0;
	int size = sizeof(currencies) / sizeof(currencies[0]);
	int i = 0;
	for (i = 0; i < size; i++) {
		res->num_currencies++;
		strcpy(res->CurrencyCodes[i], currencies[i]);
	}

	// DEBUG("[GetSupportedCurrencies] completed request\n");
	return;
}

// static void PrintSupportedCurrencies (struct http_transaction *in) {
// 	DEBUG("Supported Currencies: ");
// 	int i = 0;
// 	for (i = 0; i < in->get_supported_currencies_response.num_currencies; i++) {
// 		DEBUG("%s\t", in->get_supported_currencies_response.CurrencyCodes[i]);
// 	}
// 	DEBUG("\n");
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
	DEBUG("[Convert] received request\n");
	CurrencyConversionRequest* in = &rr->req;
	Money* euros = &rr->res;

	DEBUG("Requested conversion from %s\n", rr->req.From.CurrencyCode);
	DEBUG("Requested conversion to %s\n", rr->req.ToCode);

	// Convert: from_currency --> EUR
	void* data;
	find_c_map(currency_data_map, in->From.CurrencyCode, &data);
	euros->Units = (int64_t)((double)in->From.Units/ *(double*)data);
	euros->Nanos = (int32_t)((double)in->From.Nanos/ *(double*)data);

	DEBUG("A\n");

	Carry(euros);
	euros->Nanos = (int32_t)(round((double) euros->Nanos));

	DEBUG("B\n");

	// Convert: EUR --> to_currency
	find_c_map(currency_data_map, in->ToCode, &data);
	euros->Units = (int64_t)((double)euros->Units/ *(double*)data);
	euros->Nanos = (int32_t)((double)euros->Nanos/ *(double*)data);
	Carry(euros);

	DEBUG("C\n");

	euros->Units = (int64_t)(floor((double)(euros->Units)));
	euros->Nanos = (int32_t)(floor((double)(euros->Nanos)));
	strcpy(euros->CurrencyCode, in->ToCode);

	DEBUG("[Convert] completed request\n");
	return;
}

static void handle_request(struct unimsg_shm_desc *descs,
			   unsigned *ndescs __unused)
{
	CurrencyRpc *rpc = descs[0].addr;

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
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	currency_data_map = new_c_map(compare_e, NULL, NULL);
	getCurrencyData(currency_data_map);

	run_service(CURRENCY_SERVICE, handle_request);
	
	return 0;
}
