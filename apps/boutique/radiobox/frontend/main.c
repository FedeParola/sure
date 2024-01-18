// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
#include "../common/service.h"
#include "../common/messages.h"

#define HTTP_ERROR() ({ fprintf(stderr, "HTTP error\n"); exit(0); })
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\n"				\
                      "Connection: close\r\n"				\
                      "Content-Type: text/plain\r\n"			\
                      "Content-Length: 13\r\n"				\
                      "\r\n"						\
                      "Hello World\r\n"

#define USER_ID "federico"

static char currency[] = "CAD";
static int dependencies[] = {
	AD_SERVICE,
	CURRENCY_SERVICE,
	SHIPPING_SERVICE,
	PRODUCTCATALOG_SERVICE,
	CART_SERVICE,
	RECOMMENDATION_SERVICE,
	CHECKOUT_SERVICE
};
static struct unimsg_sock *socks[NUM_SERVICES];

// static void setCurrencyHandler(struct http_transaction *txn) {
// 	printf("Call setCurrencyHandler\n");
// 	char* query = httpQueryParser(txn->request);
// 	char _defaultCurrency[5] = "CAD";
// 	strcpy(_defaultCurrency, strchr(query, '=') + 1);

// 	txn->hop_count += 100;
// 	txn->next_fn = GATEWAY; // Hack: force gateway to return a response
// }

static GetSupportedCurrenciesResponse *
getCurrencies(struct unimsg_shm_desc *desc)
{
	int rc;

	CurrencyRpc *currency_rpc = desc->addr;
	currency_rpc->command = CURRENCY_COMMAND_GET_SUPPORTED_CURRENCIES;
	desc->size = sizeof(CurrencyRpc)
		     + sizeof(GetSupportedCurrenciesResponse);

	rc = unimsg_send(socks[CURRENCY_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CURRENCY_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size
	    != sizeof(CurrencyRpc) + sizeof(GetSupportedCurrenciesResponse)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	currency_rpc = desc->addr;
	return (GetSupportedCurrenciesResponse *)currency_rpc->rr;
}

static ListProductsResponse *getProducts(struct unimsg_shm_desc *desc)
{
	int rc;

	ProductCatalogRpc *rpc = desc->addr;
	rpc->command = PRODUCT_CATALOG_COMMAND_LIST_PRODUCTS;
	desc->size = sizeof(ProductCatalogRpc) + sizeof(ListProductsResponse);

	rc = unimsg_send(socks[PRODUCTCATALOG_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[PRODUCTCATALOG_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size
	    != sizeof(ProductCatalogRpc) + sizeof(ListProductsResponse)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rpc = desc->addr;
	return (ListProductsResponse *)rpc->rr;
}

static Cart *getCart(struct unimsg_shm_desc *desc, char *user_id)
{
	int rc;

	CartRpc *cart_rpc = desc->addr;
	cart_rpc->command = CART_COMMAND_GET_CART;
	desc->size = sizeof(CartRpc) + sizeof(GetCartRR);
	GetCartRR *get_cart_rr = (GetCartRR *)cart_rpc->rr;
	strcpy(get_cart_rr->req.UserId, user_id);

	rc = unimsg_send(socks[CART_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CART_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(CartRpc) + sizeof(GetCartRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	return &((GetCartRR *)((CartRpc *)desc->addr)->rr)->res;
}

static Money convertCurrency(struct unimsg_shm_desc *desc, Money price_usd,
			     char *user_currency)
{
	int rc;

	CurrencyRpc *currency_rpc = desc->addr;
	currency_rpc->command = CURRENCY_COMMAND_CONVERT;
	desc->size = sizeof(CurrencyRpc) + sizeof(CurrencyConversionRR);
	CurrencyConversionRR *conv_rr =
		(CurrencyConversionRR *)currency_rpc->rr;
	conv_rr->req.From = price_usd;
	strcpy(conv_rr->req.ToCode, user_currency);

	rc = unimsg_send(socks[CURRENCY_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CURRENCY_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(CurrencyRpc) + sizeof(CurrencyConversionRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	return ((CurrencyConversionRR *)((CurrencyRpc *)desc->addr)->rr)->res;
}

static AdResponse *getAd(struct unimsg_shm_desc *desc, char *ctx_keys[],
			 unsigned num_ctx_keys)
{
	int rc;

	AdRR *rr = desc->addr;
	desc->size = sizeof(AdRR);
	for (unsigned i = 0; i < num_ctx_keys; i++)
		strcpy(rr->req.ContextKeys[i], ctx_keys[i]);
	rr->req.num_context_keys = num_ctx_keys;

	rc = unimsg_send(socks[AD_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[AD_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(AdRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rr = desc->addr;
	return &rr->res;
}

static Ad *chooseAd(struct unimsg_shm_desc *desc, char *ctx_keys[],
		    unsigned num_ctx_keys)
{
	AdResponse *ads = getAd(desc, ctx_keys, num_ctx_keys);

	return &ads->Ads[rand() % ads->num_ads];
}

static void homeHandler(struct unimsg_shm_desc *desc)
{
	/* Discard result */
	getCurrencies(desc);
	
	/* Discard result */
	getCart(desc, "default_user_id");

	ListProductsResponse *products = getProducts(desc);

	for (int i = 0; i < products->num_products; i++) {
		/* Discard result */
		convertCurrency(desc, products->Products[i].PriceUsd, currency);
	}

	chooseAd(desc, NULL, 0);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static Product getProduct(struct unimsg_shm_desc *desc, char *product_id)
{
	int rc;

	ProductCatalogRpc *rpc = desc->addr;
	rpc->command = PRODUCT_CATALOG_COMMAND_GET_PRODUCT;
	desc->size = sizeof(ProductCatalogRpc) + sizeof(GetProductRR);
	GetProductRR *rr = (GetProductRR *)rpc->rr;
	strcpy(rr->req.Id, product_id);

	rc = unimsg_send(socks[PRODUCTCATALOG_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[PRODUCTCATALOG_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(ProductCatalogRpc) + sizeof(GetProductRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	return ((GetProductRR *)(((ProductCatalogRpc *)desc->addr)->rr))->res;
}

static ListRecommendationsResponse *
getRecommendations(struct unimsg_shm_desc *desc, char *user_id,
		   char *product_ids[], unsigned num_product_ids)
{
	int rc;

	printf("Need to convert id %s\n", *product_ids);
	printf("Need to convert id %s\n", product_ids[0]);

	ListRecommendationsRR *rr = desc->addr;
	desc->size = sizeof(ListRecommendationsRR);
	strcpy(rr->req.user_id, user_id);
	for (unsigned i = 0; i < num_product_ids; i++) {
		printf("Gonna copy %p:'%s' to %p\n", product_ids[i], product_ids[i], rr->req.product_ids[i]);
		strcpy(rr->req.product_ids[i], product_ids[i]);
		printf("Copied\n");
	}
	rr->req.num_product_ids = num_product_ids;

	rc = unimsg_send(socks[RECOMMENDATION_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[RECOMMENDATION_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(ListRecommendationsRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	return &((ListRecommendationsRR *)desc->addr)->res;
}

static void productHandler(struct unimsg_shm_desc *desc, char *arg)
{
	Product p = getProduct(desc, arg);

	/* Discard result */
	getCurrencies(desc);

	/* Discard result */
	getCart(desc, "default_user_id");

	/* Discard result */
	convertCurrency(desc, p.PriceUsd, currency);

	/* Discard result */
	char *product_id = p.Id;
	getRecommendations(desc, USER_ID, &product_id, 1);

	/* Discard result */
	chooseAd(desc, NULL, 0);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static Money getShippingQuote(struct unimsg_shm_desc *desc,
			      CartItem *items, unsigned num_items,
			      char *currency)
{
	int rc;

	ShippingRpc *rpc = desc->addr;
	desc->size = sizeof(ShippingRpc) + sizeof(GetQuoteRR);
	rpc->command = SHIPPING_COMMAND_GET_QUOTE;
	GetQuoteRR *rr = (GetQuoteRR *)rpc->rr;

	memset(&rr->req.address, 0, sizeof(rr->req.address));
	for (unsigned i = 0; i < num_items; i++)
		rr->req.Items[i] = items[i];
	rr->req.num_items = num_items;

	rc = unimsg_send(socks[SHIPPING_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[SHIPPING_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(ShippingRpc) + sizeof(GetQuoteRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rpc = desc->addr;
	rr = (GetQuoteRR *)rpc->rr;
	Money localized = convertCurrency(desc, rr->res.CostUsd, currency);

	return localized;
}

#define NANOSMOD 1000000000

static void MoneySum(Money *total, Money *add)
{
	total->Units = total->Units + add->Units;
	total->Nanos = total->Nanos + add->Nanos;

	if ((total->Units == 0 && total->Nanos == 0)
	    || (total->Units > 0 && total->Nanos >= 0)
	    || (total->Units < 0 && total->Nanos <= 0)) {
		// same sign <units, nanos>
		total->Units += (int64_t)(total->Nanos / NANOSMOD);
		total->Nanos = total->Nanos % NANOSMOD;
	} else {
		// different sign. nanos guaranteed to not to go over the limit
		if (total->Units > 0) {
			total->Units--;
			total->Nanos += NANOSMOD;
		} else {
			total->Units++;
			total->Nanos -= NANOSMOD;
		}
	}
}

static void MoneyMultiplySlow(Money *total, uint32_t n)
{
	for (; n > 1 ;) {
		MoneySum(total, total);
		n--;
	}
}

static void viewCartHandler(struct unimsg_shm_desc *desc)
{
	/* Discard result */
	getCurrencies(desc);

	Cart cart = *getCart(desc, "default_user_id");

	char *product_ids[10];
	for (int i = 0; i < cart.num_items; i++)
		product_ids[i] = cart.Items[i].ProductId;

	/* Discard result */
	getRecommendations(desc, USER_ID, product_ids, cart.num_items);

	Money shipping_cost = getShippingQuote(desc, cart.Items, cart.num_items,
					       currency);

	Money total_price = {0};
	for (int i = 0; i < cart.num_items; i++) {
		Product p = getProduct(desc, cart.Items[i].ProductId);
		Money price = convertCurrency(desc, p.PriceUsd, currency);
		MoneyMultiplySlow(&price, cart.Items[i].Quantity);
		MoneySum(&total_price, &price);
	}
	MoneySum(&total_price, &shipping_cost);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static void insertCart(struct unimsg_shm_desc *desc, char *user_id,
		       char *product_id, int quantity)
{
	int rc;

	CartRpc *rpc = desc->addr;
	desc->size = sizeof(CartRpc) + sizeof(AddItemRequest);
	rpc->command = CART_COMMAND_ADD_ITEM;
	AddItemRequest *req = (AddItemRequest *)rpc->rr;

	strcpy(req->Item.ProductId, product_id);
	req->Item.Quantity = quantity;
	strcpy(req->UserId, user_id);

	rc = unimsg_send(socks[CART_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CART_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(CartRpc) + sizeof(AddItemRequest)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

#define ADD_TO_CART_QUANTITY 5
#define ADD_TO_CART_PRODUCT_ID "OLJCESPC7Z"

static void addToCartHandler(struct unimsg_shm_desc *desc, char *body)
{
	Product p = getProduct(desc, ADD_TO_CART_PRODUCT_ID);

	insertCart(desc, USER_ID, p.Id, ADD_TO_CART_QUANTITY);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static void emptyCart(struct unimsg_shm_desc *desc, char *user_id)
{
	int rc;

	CartRpc *rpc = desc->addr;
	desc->size = sizeof(CartRpc) + sizeof(EmptyCartRequest);
	rpc->command = CART_COMMAND_EMPTY_CART;
	EmptyCartRequest *req = (EmptyCartRequest *)rpc->rr;

	strcpy(req->UserId, user_id);

	rc = unimsg_send(socks[CART_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CART_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(CartRpc) + sizeof(EmptyCartRequest)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

static void emptyCartHandler(struct unimsg_shm_desc *desc)
{
	emptyCart(desc, USER_ID);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

#define SET_CURRENCY_CURRENCY "EUR"

static void setCurrencyHandler(struct unimsg_shm_desc *desc, char *body)
{
	strcpy(currency, SET_CURRENCY_CURRENCY);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static void logoutHandler(struct unimsg_shm_desc *desc)
{
	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

#define PLACE_ORDER_EMAIL		"someone@example.com"
#define PLACE_ORDER_STREET_ADDRESS	"1600 Amphitheatre Parkway"
#define PLACE_ORDER_ZIP_CODE		94043
#define PLACE_ORDER_CITY		"Mountain View"
#define PLACE_ORDER_STATE		"CA"
#define PLACE_ORDER_COUNTRY		"United States"
#define PLACE_ORDER_CC_NUMBER		"4432-8015-6152-0454"
#define PLACE_ORDER_CC_MONTH		1
#define PLACE_ORDER_CC_YEAR		2039
#define PLACE_ORDER_CC_CVV		672

static void placeOrderHandler(struct unimsg_shm_desc *desc, char *body)
{
	int rc;

	PlaceOrderRR *rr = desc->addr;
	desc->size = sizeof(PlaceOrderRR);

	strcpy(rr->req.Email, PLACE_ORDER_EMAIL);
	strcpy(rr->req.address.StreetAddress, PLACE_ORDER_STREET_ADDRESS);
	rr->req.address.ZipCode = PLACE_ORDER_ZIP_CODE;
	strcpy(rr->req.address.City, PLACE_ORDER_CITY);
	strcpy(rr->req.address.State, PLACE_ORDER_STATE);
	strcpy(rr->req.address.Country, PLACE_ORDER_COUNTRY);
	strcpy(rr->req.CreditCard.CreditCardNumber, PLACE_ORDER_CC_NUMBER);
	rr->req.CreditCard.CreditCardExpirationMonth = PLACE_ORDER_CC_MONTH;
	rr->req.CreditCard.CreditCardExpirationYear = PLACE_ORDER_CC_YEAR;
	rr->req.CreditCard.CreditCardCvv = PLACE_ORDER_CC_CVV;

	rc = unimsg_send(socks[CHECKOUT_SERVICE], desc, 1, 0); 
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[CART_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(PlaceOrderRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rr = desc->addr;

	/* Discard result */
	getRecommendations(desc, USER_ID, NULL, 0);

	Money total_paid = rr->res.order.ShippingCost;
	for (unsigned i = 0; i < rr->res.order.num_items; i++) {
		Money mult_price = rr->res.order.Items[i].Cost;
		MoneyMultiplySlow(&mult_price,
				  rr->res.order.Items[i].Item.Quantity);
		MoneySum(&total_paid, &mult_price);
	}

	/* Discard result */
	getCurrencies(desc);

	strcpy(desc->addr, HTTP_RESPONSE);
	desc->size = strlen(desc->addr);
}

static void parse_http_request(struct unimsg_shm_desc *desc, char **method,
			       char **url, char **body)
{
	char *msg = desc->addr;

	/* TODO: handle desc->size == BUFFER_SIZE */
	msg[desc->size] = 0;

	char *line_end = strstr(msg, "\r\n");
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

	*body = strstr(line_end + 1, "\r\n\r\n");
	if (!*body)
		HTTP_ERROR();
	*body += 4;
}

static void handle_http_request(struct unimsg_shm_desc *desc)
{
	char *method, *url, *body;

	parse_http_request(desc, &method, &url, &body);

	int handled = 0;

	printf("Handling %s %s\n", method, url);

	/* Request routing */
	if (!strcmp(url, "/")) {
		if (!strcmp(method, "GET")) {
			homeHandler(desc);
			handled = 1;
		}
	} else if (!strncmp(url, "/product/", sizeof("/product/") - 1)) {
		if (!strcmp(method, "GET")) {
			productHandler(desc, url + sizeof("/product/") - 1);
			handled = 1;
		}
	} else if (!strcmp(url, "/cart")) {
		if (!strcmp(method, "GET")) {
			viewCartHandler(desc);
			handled = 1;
		} else if (!strcmp(method, "POST")) {
			addToCartHandler(desc, body);
			handled = 1;
		}
	} else if (!strcmp(url, "/cart/empty")) {
		if (!strcmp(method, "POST")) {
			emptyCartHandler(desc);
			handled = 1;
		}
	} else if (!strcmp(url, "/setCurrency")) {
		if (!strcmp(method, "POST")) {
			setCurrencyHandler(desc, body);
			handled = 1;
		}
	} else if (!strcmp(url, "/logout")) {
		if (!strcmp(method, "GET")) {
			logoutHandler(desc);
			handled = 1;
		}
	} else if (!strcmp(url, "/cart/checkout")) {
		if (!strcmp(method, "POST")) {
			placeOrderHandler(desc, body);
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

	/* Connect to services */
	for (unsigned i = 0; i < sizeof(dependencies) / sizeof(dependencies[0]);
	     i++) {
		unsigned id = dependencies[i];

		rc = unimsg_socket(&socks[id]);
		if (rc) {
			fprintf(stderr, "Error creating unimsg socket: %s\n",
				strerror(-rc));
			return 1;
		}

		rc = unimsg_connect(socks[id], services[id].addr,
				    services[id].port);
		if (rc) {
			fprintf(stderr, "Error connecting to %s service: %s\n",
				services[id].name, strerror(-rc));
			return 1;
		}

		printf("Connected to %s service\n", services[id].name);
	}

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
