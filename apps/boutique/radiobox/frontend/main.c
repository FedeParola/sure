// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <rte_branch_prediction.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_memzone.h>

#include "http.h"
#include "io.h"
#include "spright.h"
#include "utility.h"
#include "shm_rpc.h"

static int pipefd_rx[UINT8_MAX][2];
static int pipefd_tx[UINT8_MAX][2];

// char defaultCurrency[5] = "CAD";

static void setCurrencyHandler(struct http_transaction *txn) {
	printf("Call setCurrencyHandler\n");
	char* query = httpQueryParser(txn->request);
	char _defaultCurrency[5] = "CAD";
	strcpy(_defaultCurrency, strchr(query, '=') + 1);

	txn->hop_count += 100;
	txn->next_fn = GATEWAY; // Hack: force gateway to return a response
}

static void homeHandler(struct http_transaction *txn) {
	printf("Call homeHandler ### Hop: %u\n", txn->hop_count);

	if (txn->hop_count == 0) {
		getCurrencies(txn);

	} else if (txn->hop_count == 1) {
		getProducts(txn);
		txn->productViewCntr = 0;

	} else if (txn->hop_count == 2) {
		getCart(txn);

	} else if (txn->hop_count == 3) {
		convertCurrencyOfProducts(txn);
		homeHandler(txn);
	} else if (txn->hop_count == 4) {
		chooseAd(txn);

	} else if (txn->hop_count == 5) {
		returnResponse(txn);

	} else {
		printf("homeHandler doesn't know what to do for HOP %u.\n", txn->hop_count);
		returnResponse(txn);

	}
	return;
}

static void productHandler(struct http_transaction *txn) {
	printf("Call productHandler ### Hop: %u\n", txn->hop_count);

	if (txn->hop_count == 0) {
		getProduct(txn);
		txn->productViewCntr = 0;
	} else if (txn->hop_count == 1) {
		getCurrencies(txn);
	} else if (txn->hop_count == 2) {
		getCart(txn);
	} else if (txn->hop_count == 3) {
		convertCurrencyOfProduct(txn);

	} else if (txn->hop_count == 4) {
		chooseAd(txn);
	} else if (txn->hop_count == 5) {
		returnResponse(txn);
	} else {
		printf("productHandler doesn't know what to do for HOP %u.\n", txn->hop_count);
		returnResponse(txn);

	}
	return;
}

static void addToCartHandler(struct http_transaction *txn) {
	printf("Call addToCartHandler ### Hop: %u\n", txn->hop_count);
	if (txn->hop_count == 0) {
		getProduct(txn);
		txn->productViewCntr = 0;

	} else if (txn->hop_count == 1) {
		insertCart(txn);

	} else if (txn->hop_count == 2) {
		returnResponse(txn);
	} else {
		printf("addToCartHandler doesn't know what to do for HOP %u.\n", txn->hop_count);
		returnResponse(txn);
	}
}

static void viewCartHandler(struct http_transaction *txn) {
	printf("[%s()] Call viewCartHandler ### Hop: %u\n", __func__, txn->hop_count);
	if (txn->hop_count == 0) {
		getCurrencies(txn);

	} else if (txn->hop_count == 1) {
		getCart(txn);
		txn->cartItemViewCntr = 0;
		strcpy(txn->total_price.CurrencyCode, defaultCurrency);

	} else if (txn->hop_count == 2) {
		getRecommendations(txn);

	} else if (txn->hop_count == 3) {
		getShippingQuote(txn);

	} else if (txn->hop_count == 4) {
		convertCurrencyOfShippingQuote(txn);
		if (txn->get_quote_response.conversion_flag == true) {
			getCartItemInfo(txn);
			txn->hop_count++;

		} else {
			printf("[%s()] Set get_quote_response.conversion_flag as true\n", __func__);
			txn->get_quote_response.conversion_flag = true;
		}
		
	} else if (txn->hop_count == 5) {
		getCartItemInfo(txn);

	} else if (txn->hop_count == 6) {
		convertCurrencyOfCart(txn);
	} else {
		printf("[%s()] viewCartHandler doesn't know what to do for HOP %u.\n", __func__, txn->hop_count);
		returnResponse(txn);
	}
}

static void PlaceOrder(struct http_transaction *txn) {
	parsePlaceOrderRequest(txn);
	// PrintPlaceOrderRequest(txn);

	strcpy(txn->rpc_handler, "PlaceOrder");
	txn->caller_fn = FRONTEND;
	txn->next_fn = CHECKOUT_SVC;
	txn->hop_count++;
	txn->checkoutsvc_hop_cnt = 0;
}

static void placeOrderHandler(struct http_transaction *txn) {
	printf("[%s()] Call placeOrderHandler ### Hop: %u\n", __func__, txn->hop_count);

	if (txn->hop_count == 0) {
		PlaceOrder(txn);

	} else if (txn->hop_count == 1) {
		getRecommendations(txn);

	} else if (txn->hop_count == 2) {
		getCurrencies(txn);

	} else if (txn->hop_count == 3) {
		returnResponse(txn);

	} else {
		printf("[%s()] placeOrderHandler doesn't know what to do for HOP %u.\n", __func__, txn->hop_count);
		returnResponse(txn);
	}
}

#define HTTP_ERROR() exit(0)

static void parse_http_request(struct unimsg_shm_desc *desc, char **method,
			       char **url, char **body)
{
	char *msg = desc->addr;
	msg[desc->size] = 0;

	char *line_end = strchr(msg, '\r');
	if (!line_end)
		HTTP_ERROR();

	*line_end = 0;

	char *method_end = strchr(msg, ' ');
	if (!method_end)
		HTTP_ERROR();
	*method_end = 0;

	*method = msg;
	char *next = method_end + 1;

	char *url_end = strchr(next, ' ');
	if (!url_end)
		HTTP_ERROR();
	*url_end = 0;

	*url = next;
}

static void handle_http_request(struct unimsg_shm_desc *desc)
{
	char *method, *url, *body;

	parse_http_request(desc, &method, &url, &body);

	int handled = 0;

	/* Request routing */
	if (!strcmp(url, "/")) {
		if (!strcmp(method, "GET")) {
			homeHandler();
			handled = 1;
		}
	} else if (!strncmp(url, "/product/", sizeof("/product/") - 1)) {
		if (!strcmp(method, "GET")) {
			productHandler();
			handled = 1;
		}
	} else if (!strcmp(url, "/cart")) {
		if (!strcmp(method, "GET")) {
			viewCartHandler();
			handled = 1;
		} else if (!strcmp(method, "POST")) {
			addToCartHandler();
			handled = 1;
		}
	} else if (!strcmp(url, "/cart/empty")) {
		if (!strcmp(method, "POST")) {
			emptyCartHandler();
			handled = 1;
		}
	} else if (!strcmp(url, "/setCurrency")) {
		if (!strcmp(method, "POST")) {
			setCurrencyHandler();
			handled = 1;
		}
	} else if (!strcmp(url, "/logout")) {
		if (!strcmp(method, "GET")) {
			logoutHandler();
			handled = 1;
		}
	} else if (!strcmp(url, "/cart/checkout")) {
		if (!strcmp(method, "POST")) {
			placeOrderHandler();
			handled = 1;
		}
	}

	if (!handled)
		HTTP_ERROR();
}

int main(int argc, char **argv)
{
	int rc;
	struct unimsg_sock *s;

	(void)argc;
	(void)argv;

	rc = unimsg_socket(&s);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		return 1;
	}

	rc = unimsg_bind(s, FRONTEND_PORT);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n", FRONTEND_PORT,
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

		nrecv = 1;
		rc = unimsg_recv(cs, &desc, &nrecv, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		handle_http_request(&desc);

		rc = unimsg_send(cs, &desc, 1, 0);
		if (rc) {
			fprintf(stderr, "Error sending desc: %s\n",
				strerror(-rc));
			ERR_PUT(&desc, 1, s);
		}
	}
	
	return 0;
}
