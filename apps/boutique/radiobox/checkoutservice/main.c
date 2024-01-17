// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include "../common/service.h"
#include "../common/messages.h"

#define DEFAULT_UUID "1b4e28ba-2fa1-11d2-883f-0016d3cca427"
#define NANOSMOD 1000000000
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static int dependencies[] = {
	PRODUCTCATALOG_SERVICE,
	CART_SERVICE,
	SHIPPING_SERVICE,
	CURRENCY_SERVICE,
	PAYMENT_SERVICE,
	EMAIL_SERVICE,
};
static struct unimsg_sock *socks[NUM_SERVICES];

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

	return;
}

void MoneyMultiplySlow(Money *total, uint32_t n)
{
	for (; n > 1 ;) {
		MoneySum(total, total);
		n--;
	}
	return;
}

static Cart *getUserCart(struct unimsg_shm_desc *desc, PlaceOrderRR *rr)
{
	int rc;

	CartRpc *cart_rpc = desc->addr;
	cart_rpc->command = CART_COMMAND_GET_CART;
	desc->size = sizeof(CartRpc) + sizeof(GetCartRR);
	GetCartRR *get_cart_rr = (GetCartRR *)cart_rpc->rr;
	memcpy(get_cart_rr->req.UserId, rr->req.UserId, sizeof(rr->req.UserId));

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

static Product *getProduct(struct unimsg_shm_desc *desc, char *product_id)
{
	int rc;

	ProductCatalogRpc *catalog_rpc = desc->addr;
	catalog_rpc->command = PRODUCT_CATALOG_COMMAND_GET_PRODUCT;
	desc->size = sizeof(ProductCatalogRpc) + sizeof(GetProductRR);
	GetProductRR *get_prod_rr = (GetProductRR *)catalog_rpc->rr;
	strcpy(get_prod_rr->req.Id, product_id);

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

	return &((GetProductRR *)(((ProductCatalogRpc *)desc->addr)->rr))->res;
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

static void prepOrderItems(Cart *cart, char *user_currency,
			   OrderItem *order_items, unsigned *num_order_items)
{
	int rc;
	struct unimsg_shm_desc desc;

	rc = unimsg_buffer_get(&desc, 1); 
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	for (int i = 0; i < cart->num_items; i++) {
		order_items[i].Item = cart->Items[i];

		Product *prod = getProduct(&desc, cart->Items[i].ProductId);

		order_items[i].Cost = convertCurrency(&desc, prod->PriceUsd,
						      user_currency);
	}

	unimsg_buffer_put(&desc, 1);

	*num_order_items = cart->num_items;
}

static Money quoteShipping(struct unimsg_shm_desc *desc, Address *address,
			   Cart *cart)
{
	int rc;

	ShippingRpc *rpc = desc->addr;
	rpc->command = SHIPPING_COMMAND_GET_QUOTE;
	desc->size = sizeof(ShippingRpc) + sizeof(GetQuoteRR);
	GetQuoteRR *rr = (GetQuoteRR *)rpc->rr;
	rr->req.address = *address;
	rr->req.num_items = cart->num_items;
	memcpy(rr->req.Items, cart->Items, sizeof(CartItem) * cart->num_items);

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

	return ((GetQuoteRR *)((ShippingRpc *)desc->addr)->rr)->res.CostUsd;
}

void prepareOrderItemsAndShippingQuoteFromCart(PlaceOrderRR *rr,
					       OrderItem *order_items,
					       unsigned *num_order_items,
					       Money *shipping_cost)
{
	int rc;

	struct unimsg_shm_desc cart_desc;
	rc = unimsg_buffer_get(&cart_desc, 1); 
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}
	Cart *cart = getUserCart(&cart_desc, rr);

	prepOrderItems(cart, rr->req.UserCurrency, order_items,
		       num_order_items);

	struct unimsg_shm_desc ship_desc;
	rc = unimsg_buffer_get(&ship_desc, 1); 
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	Money shipping_usd = quoteShipping(&ship_desc, &rr->req.address, cart);
	*shipping_cost = convertCurrency(&ship_desc, shipping_usd,
					 rr->req.UserCurrency);

	unimsg_buffer_put(&ship_desc, 1);
	unimsg_buffer_put(&cart_desc, 1);
}

static void chargeCard(struct unimsg_shm_desc *desc, Money amount,
		       CreditCardInfo paymentInfo, char *transaction_id)
{
	int rc;

	ChargeRR *rr = desc->addr;
	desc->size = sizeof(ChargeRR);
	rr->req.Amount = amount;
	rr->req.CreditCard = paymentInfo;

	rc = unimsg_send(socks[PAYMENT_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[PAYMENT_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(ChargeRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rr = desc->addr;
	strcpy(transaction_id, rr->res.TransactionId);
}

static void shipOrder(struct unimsg_shm_desc *desc,
		      Address *address, OrderItem *items, unsigned num_items,
		      char *tracking_id)
{
	int rc;

	ShippingRpc *rpc = desc->addr;
	desc->size = sizeof(ShippingRpc) + sizeof(ShipOrderRR);
	rpc->command = SHIPPING_COMMAND_SHIP_ORDER;
	ShipOrderRR *rr = (ShipOrderRR *)rpc->rr;
	rr->req.address = *address;
	for (unsigned i = 0; i < num_items; i++)
		rr->req.Items[i] = items[i].Item;

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

	if (desc->size != sizeof(ShippingRpc) + sizeof(ShipOrderRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}

	rpc = desc->addr;
	rr = (ShipOrderRR *)rpc->rr;
	strcpy(tracking_id, rr->res.TrackingId);
}

static void emptyUserCart(struct unimsg_shm_desc *desc, char *user_id)
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


static void sendOrderConfirmation(struct unimsg_shm_desc *desc, char *email,
				  OrderResult *order)
{
	int rc;

	SendOrderConfirmationRR *rr = desc->addr;
	desc->size = sizeof(SendOrderConfirmationRR);
	strcpy(rr->req.Email , email);
	rr->req.Order = *order;

	rc = unimsg_send(socks[EMAIL_SERVICE], desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		exit(1);
	}

	unsigned nrecv = 1;
	rc = unimsg_recv(socks[EMAIL_SERVICE], desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		exit(1);
	}

	if (desc->size != sizeof(SendOrderConfirmationRR)) {
		fprintf(stderr, "Received reply of unexpected size\n");
		exit(1);
	}
}

static void PlaceOrder(PlaceOrderRR *rr)
{
	int rc;
	struct unimsg_shm_desc desc;
	unsigned num_items;
	Money total;

	/* Allocate a buffer to handle most of the requests to other services */
	rc = unimsg_buffer_get(&desc, 1); 
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	OrderResult *order = &rr->res.order;
	strcpy(order->OrderId, DEFAULT_UUID);
	order->ShippingAddress = rr->req.address;

	prepareOrderItemsAndShippingQuoteFromCart(rr, order->Items, &num_items,
						  &order->ShippingCost);

	strcpy(total.CurrencyCode, rr->req.UserCurrency);
	total.Nanos = 0;
	total.Units = 0;

	MoneySum(&total, &order->ShippingCost);
	for (unsigned i = 0; i < num_items; i++) {
		Money mult_price = order->Items[i].Cost;
		MoneyMultiplySlow(&mult_price, order->Items[i].Item.Quantity);
		MoneySum(&total, &mult_price);
	}

	char transaction_id[40];
	chargeCard(&desc, total, rr->req.CreditCard, transaction_id);

	shipOrder(&desc, &rr->req.address, order->Items, num_items,
		  order->ShippingTrackingId);

	emptyUserCart(&desc, rr->req.UserId);

	sendOrderConfirmation(&desc, rr->req.Email, order);

	unimsg_buffer_put(&desc, 1);
}

static void handle_request(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned nrecv;
	PlaceOrderRR *rr;

	nrecv = 1;
	int rc = unimsg_recv(s, &desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	rr = desc.addr;

	PlaceOrder(rr);

	rc = unimsg_send(s, &desc, 1, 0);
	if (rc) {
		fprintf(stderr, "Error sending desc: %s\n", strerror(-rc));
		ERR_PUT(&desc, 1, s);
	}
}

int main(int argc, char **argv)
{
	int rc;

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

		if (unimsg_connect(socks[id], services[id].addr,
				   services[id].port)) {
			fprintf(stderr, "Error connecting to %s service: %s\n",
				services[id].name, strerror(-rc));
			ERR_CLOSE(socks[id]);
		}

		printf("Connected to %s service\n", services[id].name);
	}

	run_service(CHECKOUT_SERVICE, handle_request);
	
	return 0;
}