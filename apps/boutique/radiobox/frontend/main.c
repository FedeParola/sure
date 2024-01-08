// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#define UPSTREAM_HTTP 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
#include "../common/service/service_async.h"
#include "../common/service/utilities.h"

#define HTTP_ERROR() ({ fprintf(stderr, "HTTP error\n"); exit(0); })
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

#define HTTP_OK "HTTP/1.1 200 OK\r\n"					\
		"Content-Length: 0\r\n"					\
		"\r\n"

#define HTTP_BAD_REQUEST "HTTP/1.1 400 Bad Request\r\n"			\
			 "Content-Length: 0\r\n"			\
			 "\r\n"

#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\n"			\
		       "Content-Length: 0\r\n"				\
		       "\r\n"

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

static GetSupportedCurrenciesResponse *
getCurrencies(struct unimsg_shm_desc *desc)
{
	DEBUG("getCurrencies()\n");

	struct rpc *rpc = desc->addr;
	rpc->command = CURRENCY_GET_SUPPORTED_CURRENCIES;
	desc->size = get_rpc_size(CURRENCY_GET_SUPPORTED_CURRENCIES);

	do_rpc(desc, CURRENCY_SERVICE);

	DEBUG("/getCurrencies()\n");

	rpc = desc->addr;
	return (GetSupportedCurrenciesResponse *)rpc->rr;
}

static ListProductsResponse *getProducts(struct unimsg_shm_desc *desc)
{
	struct rpc *rpc = desc->addr;
	rpc->command = PRODUCTCATALOG_LIST_PRODUCTS;
	desc->size = get_rpc_size(PRODUCTCATALOG_LIST_PRODUCTS);

	do_rpc(desc, PRODUCTCATALOG_SERVICE);

	rpc = desc->addr;
	return (ListProductsResponse *)rpc->rr;
}

static Cart *getCart(struct unimsg_shm_desc *desc, char *user_id)
{
	struct rpc *rpc = desc->addr;
	rpc->command = CART_GET_CART;
	desc->size = get_rpc_size(CART_GET_CART);
	GetCartRR *get_cart_rr = (GetCartRR *)rpc->rr;
	strcpy(get_cart_rr->req.UserId, user_id);

	do_rpc(desc, CART_SERVICE);

	return &((GetCartRR *)((struct rpc *)desc->addr)->rr)->res;
}

static Money convertCurrency(struct unimsg_shm_desc *desc, Money price_usd,
			     char *user_currency)
{
	struct rpc *rpc = desc->addr;
	rpc->command = CURRENCY_CONVERT;
	desc->size = get_rpc_size(CURRENCY_CONVERT);
	CurrencyConversionRR *conv_rr = (CurrencyConversionRR *)rpc->rr;
	conv_rr->req.From = price_usd;
	strcpy(conv_rr->req.ToCode, user_currency);

	DEBUG("Requesting currency conversion from '%s' to '%s'\n",
	      price_usd.CurrencyCode, user_currency);

	do_rpc(desc, CURRENCY_SERVICE);

	return ((CurrencyConversionRR *)((struct rpc *)desc->addr)->rr)->res;
}

static AdResponse *getAd(struct unimsg_shm_desc *desc, char *ctx_keys[],
			 unsigned num_ctx_keys)
{
	struct rpc *rpc = desc->addr;
	rpc->command = AD_GET_ADS;
	desc->size = get_rpc_size(AD_GET_ADS);
	AdRR *rr = (AdRR *)rpc->rr;
	for (unsigned i = 0; i < num_ctx_keys; i++)
		strcpy(rr->req.ContextKeys[i], ctx_keys[i]);
	rr->req.num_context_keys = num_ctx_keys;

	do_rpc(desc, AD_SERVICE);

	rr = (AdRR *)((struct rpc *)desc->addr)->rr;
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
	getCart(desc, USER_ID);

	ListProductsResponse *products = getProducts(desc);

	DEBUG("Retrieved %d products from catalog\n", products->num_products);

	struct unimsg_shm_desc desc1;
	int rc = unimsg_buffer_get(&desc1, 1);
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	for (int i = 0; i < products->num_products; i++) {
		/* Discard result */
		convertCurrency(&desc1, products->Products[i].PriceUsd,
				currency);
	}

	unimsg_buffer_put(&desc1, 1);

	chooseAd(desc, NULL, 0);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static Product getProduct(struct unimsg_shm_desc *desc, char *product_id)
{
	struct rpc *rpc = desc->addr;
	rpc->command = PRODUCTCATALOG_GET_PRODUCT;
	desc->size = get_rpc_size(PRODUCTCATALOG_GET_PRODUCT);
	GetProductRR *rr = (GetProductRR *)rpc->rr;
	strcpy(rr->req.Id, product_id);

	do_rpc(desc, PRODUCTCATALOG_SERVICE);

	return ((GetProductRR *)(((struct rpc *)desc->addr)->rr))->res;
}

static ListRecommendationsResponse *
getRecommendations(struct unimsg_shm_desc *desc, char *user_id,
		   char *product_ids[], unsigned num_product_ids)
{
	DEBUG("getRecommendations()\n");

	struct rpc *rpc = desc->addr;
	rpc->command =  RECOMMENDATION_LIST_RECOMMENDATIONS;
	desc->size = get_rpc_size(RECOMMENDATION_LIST_RECOMMENDATIONS);
	ListRecommendationsRR *rr = (ListRecommendationsRR *)rpc->rr;
	strcpy(rr->req.user_id, user_id);
	for (unsigned i = 0; i < num_product_ids; i++)
		strcpy(rr->req.product_ids[i], product_ids[i]);
	rr->req.num_product_ids = num_product_ids;

	do_rpc(desc, RECOMMENDATION_SERVICE);

	DEBUG("/getRecommendations()\n");

	rpc = desc->addr;
	return &((ListRecommendationsRR *)rpc->rr)->res;
}

static void productHandler(struct unimsg_shm_desc *desc, char *id)
{
	Product p = getProduct(desc, id);

	/* Discard result */
	getCurrencies(desc);

	/* Discard result */
	getCart(desc, USER_ID);

	/* Discard result */
	convertCurrency(desc, p.PriceUsd, currency);

	/* Discard result */
	char *product_id = p.Id;
	getRecommendations(desc, USER_ID, &product_id, 1);

	/* Discard result */
	chooseAd(desc, NULL, 0);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static Money getShippingQuote(struct unimsg_shm_desc *desc,
			      CartItem *items, unsigned num_items,
			      char *currency)
{
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(SHIPPING_GET_QUOTE);
	rpc->command = SHIPPING_GET_QUOTE;
	GetQuoteRR *rr = (GetQuoteRR *)rpc->rr;

	memset(&rr->req.address, 0, sizeof(rr->req.address));
	for (unsigned i = 0; i < num_items; i++)
		rr->req.Items[i] = items[i];
	rr->req.num_items = num_items;

	do_rpc(desc, SHIPPING_SERVICE);

	rpc = desc->addr;
	rr = (GetQuoteRR *)rpc->rr;
	Money localized = convertCurrency(desc, rr->res.CostUsd, currency);

	return localized;
}

static void viewCartHandler(struct unimsg_shm_desc *desc)
{
	/* Discard result */
	getCurrencies(desc);

	Cart cart = *getCart(desc, USER_ID);

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

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static void insertCart(struct unimsg_shm_desc *desc, char *user_id,
		       char *product_id, int quantity)
{
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(CART_ADD_ITEM);
	rpc->command = CART_ADD_ITEM;
	AddItemRequest *req = (AddItemRequest *)rpc->rr;

	strcpy(req->Item.ProductId, product_id);
	req->Item.Quantity = quantity;
	strcpy(req->UserId, user_id);

	do_rpc(desc, CART_SERVICE);
}

static void addToCartHandler(struct unimsg_shm_desc *desc, char *body)
{
	char product_id[11];
	int quantity;

	if (sscanf(body, "product_id=%10s&quantity=%d", product_id, &quantity)
	    != 2) {
		strcpy(desc->addr, HTTP_BAD_REQUEST);
		desc->size = strlen(desc->addr);
		return;
	}

	DEBUG("Adding %d units of %s to cart\n", quantity, product_id);

	/* Discard result */
	getProduct(desc, product_id);

	insertCart(desc, USER_ID, product_id, quantity);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static void emptyCart(struct unimsg_shm_desc *desc, char *user_id)
{
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(CART_EMPTY_CART);
	rpc->command = CART_EMPTY_CART;
	EmptyCartRequest *req = (EmptyCartRequest *)rpc->rr;

	strcpy(req->UserId, user_id);

	do_rpc(desc, CART_SERVICE);
}

static void emptyCartHandler(struct unimsg_shm_desc *desc)
{
	emptyCart(desc, USER_ID);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static void setCurrencyHandler(struct unimsg_shm_desc *desc, char *body)
{
	char curr[4];
	if (sscanf(body, "currency_code=%3s", curr) != 1) {
		strcpy(desc->addr, HTTP_BAD_REQUEST);
		desc->size = strlen(desc->addr);
		return;
	}

	strcpy(currency, curr);
	DEBUG("Currency set to %s\n", currency);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static void logoutHandler(struct unimsg_shm_desc *desc)
{
	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static void placeOrderHandler(struct unimsg_shm_desc *desc, char *body __unused)
{
	int rc;

	/* TODO: convert special characters */

	char *param;
	char *param_end = body - 1;

#define READ_PARAM(name, specifier, dst)				\
	if (!param_end)	{						\
		strcpy(desc->addr, HTTP_BAD_REQUEST);			\
		desc->size = strlen(desc->addr);			\
		return;							\
	}								\
	param = param_end + 1;						\
	param_end = strchr(param, '&');					\
	if (param_end)							\
		*param_end = 0;						\
	if (sscanf(param, name "=" specifier, dst) != 1) {		\
		strcpy(desc->addr, HTTP_BAD_REQUEST);			\
		desc->size = strlen(desc->addr);			\
		return;							\
	}

	char email[50];
	READ_PARAM("email", "%s", email);
	char street_address[50];
	READ_PARAM("street_address", "%s", street_address);
	int zip_code;
	READ_PARAM("zip_code", "%d", &zip_code);
	char city[15];
	READ_PARAM("city", "%s", city);
	char state[15];
	READ_PARAM("state", "%s", state);
	char country[15];
	READ_PARAM("country", "%s", country);
	char credit_card_number[30];
	READ_PARAM("credit_card_number", "%s", credit_card_number);
	int credit_card_expiration_month;
	READ_PARAM("credit_card_expiration_month", "%d",
		   &credit_card_expiration_month);
	int credit_card_expiration_year;
	READ_PARAM("credit_card_expiration_year", "%d",
		   &credit_card_expiration_year);
	int credit_card_cvv;
	READ_PARAM("credit_card_cvv", "%d", &credit_card_cvv);

#undef READ_PARAM

	DEBUG("Placing order:\n"
	      "  email=%s\n"
	      "  street_address=%s\n"
	      "  zip_code=%d\n"
	      "  city=%s\n"
	      "  state=%s\n"
	      "  country=%s\n"
	      "  credit_card_number=%s\n"
	      "  credit_card_expiration_month=%d\n"
	      "  credit_card_expiration_year=%d\n"
	      "  credit_card_cvv=%d\n",
	      email, street_address, zip_code, city, state, country,
	      credit_card_number, credit_card_expiration_month,
	      credit_card_expiration_year, credit_card_cvv);

	struct rpc *rpc = desc->addr;
	rpc->command =  CHECKOUT_PLACE_ORDER;
	desc->size = get_rpc_size(CHECKOUT_PLACE_ORDER);
	PlaceOrderRR *rr = (PlaceOrderRR *)rpc->rr;

	strcpy(rr->req.UserId, USER_ID);
	strcpy(rr->req.UserCurrency, currency);

	strcpy(rr->req.Email, email);
	strcpy(rr->req.address.StreetAddress, street_address);
	rr->req.address.ZipCode = zip_code;
	strcpy(rr->req.address.City, city);
	strcpy(rr->req.address.State, state);
	strcpy(rr->req.address.Country, country);
	strcpy(rr->req.CreditCard.CreditCardNumber, credit_card_number);
	rr->req.CreditCard.CreditCardExpirationMonth
		= credit_card_expiration_month;
	rr->req.CreditCard.CreditCardExpirationYear
		= credit_card_expiration_year;
	rr->req.CreditCard.CreditCardCvv = credit_card_cvv;

	do_rpc(desc, CHECKOUT_SERVICE);

	rpc = desc->addr;
	rr = (PlaceOrderRR *)rpc->rr;

	struct unimsg_shm_desc desc1;
	rc = unimsg_buffer_get(&desc1, 1);
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	/* Discard result */
	getRecommendations(&desc1, USER_ID, NULL, 0);

	unimsg_buffer_put(&desc1, 1);

	Money total_paid = rr->res.order.ShippingCost;
	for (unsigned i = 0; i < rr->res.order.num_items; i++) {
		Money mult_price = rr->res.order.Items[i].Cost;
		MoneyMultiplySlow(&mult_price,
				  rr->res.order.Items[i].Item.Quantity);
		MoneySum(&total_paid, &mult_price);
	}

	/* Discard result */
	getCurrencies(desc);

	strcpy(desc->addr, HTTP_OK);
	desc->size = strlen(desc->addr);
}

static int parse_http_request(struct unimsg_shm_desc *desc, char **method,
			      char **url, char **body)
{
	char *msg = desc->addr;

	/* TODO: handle desc->size == BUFFER_SIZE */
	msg[desc->size] = 0;

	char *line_end = strstr(msg, "\r\n");
	if (!line_end)
		return 1;

	*line_end = 0;

	char *method_end = strchr(msg, ' ');
	if (!method_end)
		return 1;
	*method_end = 0;

	*method = msg;
	char *next = method_end + 1;

	char *url_end = strchr(next, ' ');
	if (!url_end)
		return 1;
	*url_end = 0;

	*url = next;

	*body = strstr(line_end + 1, "\r\n\r\n");
	if (!*body)
		return 1;
	*body += 4;

	return 0;
}

static void handle_request(struct unimsg_shm_desc *desc)
{
	char *method, *url, *body;

	if (parse_http_request(desc, &method, &url, &body)) {
		DEBUG("Error parsing request\n");
		strcpy(desc->addr, HTTP_BAD_REQUEST);
		desc->size = strlen(desc->addr);
		return;
	}

	int handled = 0;

	DEBUG("Handling %s %s\n", method, url);
	DEBUG("BODY\n%s\n", body);

	/* Request routing */
	if (!strcmp(url, "/")) {
		if (!strcmp(method, "GET")) {
			homeHandler(desc);
			handled = 1;
		}
	} else if (!strncmp(url, "/product/", sizeof("/product/") - 1)) {
		if (!strcmp(method, "GET")) {
			char id[20];
			strncpy(id, url + sizeof("/product/") - 1,
				sizeof(id) - 1);
			productHandler(desc, id);
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

	if (!handled) {
		strcpy(desc->addr, HTTP_NOT_FOUND);
		desc->size = strlen(desc->addr);
	}
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(FRONTEND, handle_request, dependencies,
		    sizeof(dependencies) / sizeof(dependencies[0]));

	return 0;
}
