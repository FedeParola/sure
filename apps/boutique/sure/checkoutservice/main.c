// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include "../common/service/service_async.h"
#include "../common/service/utilities.h"

#define DEFAULT_UUID "1b4e28ba-2fa1-11d2-883f-0016d3cca427"
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

static Cart *getUserCart(struct unimsg_shm_desc *desc, PlaceOrderRR *rr)
{
	struct rpc *rpc = desc->addr;
	rpc->command = CART_GET_CART;
	desc->size = get_rpc_size(CART_GET_CART);
	GetCartRR *get_cart_rr = (GetCartRR *)rpc->rr;
	memcpy(get_cart_rr->req.UserId, rr->req.UserId, sizeof(rr->req.UserId));

	do_rpc(desc, CART_SERVICE);

	return &((GetCartRR *)((struct rpc *)desc->addr)->rr)->res;
}

static Product *getProduct(struct unimsg_shm_desc *desc, char *product_id)
{
	struct rpc *rpc = desc->addr;
	rpc->command = PRODUCTCATALOG_GET_PRODUCT;
	desc->size = get_rpc_size(PRODUCTCATALOG_GET_PRODUCT);
	GetProductRR *rr = (GetProductRR *)rpc->rr;
	strcpy(rr->req.Id, product_id);

	do_rpc(desc, PRODUCTCATALOG_SERVICE);

	return &((GetProductRR *)(((struct rpc *)desc->addr)->rr))->res;
}

static Money convertCurrency(struct unimsg_shm_desc *desc, Money price_usd,
			     char *user_currency)
{
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	rpc->command = CURRENCY_CONVERT;
	desc->size = get_rpc_size(CURRENCY_CONVERT);
	CurrencyConversionRR *rr = (CurrencyConversionRR *)rpc->rr;
	rr->req.From = price_usd;
	strcpy(rr->req.ToCode, user_currency);

	do_rpc(desc, CURRENCY_SERVICE);

	return ((CurrencyConversionRR *)((struct rpc *)desc->addr)->rr)->res;
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
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	rpc->command = SHIPPING_GET_QUOTE;
	desc->size = get_rpc_size(SHIPPING_GET_QUOTE);
	GetQuoteRR *rr = (GetQuoteRR *)rpc->rr;
	rr->req.address = *address;
	rr->req.num_items = cart->num_items;
	memcpy(rr->req.Items, cart->Items, sizeof(CartItem) * cart->num_items);

	do_rpc(desc, SHIPPING_SERVICE);

	return ((GetQuoteRR *)((struct rpc *)desc->addr)->rr)->res.CostUsd;
}

void prepareOrderItemsAndShippingQuoteFromCart(PlaceOrderRR *rr,
					       OrderItem *order_items,
					       unsigned *num_order_items,
					       Money *shipping_cost)
{
	int rc;

	DEBUG("Getting cart\n");
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

	DEBUG("Quoting shipping\n");
	Money shipping_usd = quoteShipping(&ship_desc, &rr->req.address, cart);
	*shipping_cost = convertCurrency(&ship_desc, shipping_usd,
					 rr->req.UserCurrency);

	unimsg_buffer_put(&ship_desc, 1);
	unimsg_buffer_put(&cart_desc, 1);
}

static void chargeCard(struct unimsg_shm_desc *desc, Money amount,
		       CreditCardInfo paymentInfo, char *transaction_id)
{
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	rpc->command = PAYMENT_CHARGE;
	desc->size = get_rpc_size(PAYMENT_CHARGE);
	ChargeRR *rr = (ChargeRR *)rpc->rr;
	rr->req.Amount = amount;
	rr->req.CreditCard = paymentInfo;

	do_rpc(desc, PAYMENT_SERVICE);

	rr = (ChargeRR *)((struct rpc *)desc->addr)->rr;
	strcpy(transaction_id, rr->res.TransactionId);
}

static void shipOrder(struct unimsg_shm_desc *desc,
		      Address *address, OrderItem *items, unsigned num_items,
		      char *tracking_id)
{
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(SHIPPING_SHIP_ORDER);
	rpc->command = SHIPPING_SHIP_ORDER;
	ShipOrderRR *rr = (ShipOrderRR *)rpc->rr;
	rr->req.address = *address;
	for (unsigned i = 0; i < num_items; i++)
		rr->req.Items[i] = items[i].Item;

	do_rpc(desc, SHIPPING_SERVICE);

	rpc = desc->addr;
	rr = (ShipOrderRR *)rpc->rr;
	strcpy(tracking_id, rr->res.TrackingId);
}

static void emptyUserCart(struct unimsg_shm_desc *desc, char *user_id)
{
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(CART_EMPTY_CART);
	rpc->command = CART_EMPTY_CART;
	EmptyCartRequest *req = (EmptyCartRequest *)rpc->rr;
	strcpy(req->UserId, user_id);

	do_rpc(desc, CART_SERVICE);
}


static void sendOrderConfirmation(struct unimsg_shm_desc *desc, char *email,
				  OrderResult *order)
{
	unimsg_buffer_reset(desc);
	struct rpc *rpc = desc->addr;
	desc->size = get_rpc_size(EMAIL_SEND_ORDER_CONFIRMATION);
	rpc->command = EMAIL_SEND_ORDER_CONFIRMATION;
	SendOrderConfirmationRR *rr = (SendOrderConfirmationRR *)rpc->rr;
	strcpy(rr->req.Email , email);
	rr->req.Order = *order;

	do_rpc(desc, EMAIL_SERVICE);
}

static void PlaceOrder(PlaceOrderRR *rr)
{
	int rc;
	struct unimsg_shm_desc desc;
	Money total;

	DEBUG("Placing order\n");

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

	prepareOrderItemsAndShippingQuoteFromCart(rr, order->Items,
						  &rr->res.order.num_items,
						  &order->ShippingCost);

	strcpy(total.CurrencyCode, rr->req.UserCurrency);
	total.Nanos = 0;
	total.Units = 0;

	MoneySum(&total, &order->ShippingCost);
	for (unsigned i = 0; i < rr->res.order.num_items; i++) {
		Money mult_price = order->Items[i].Cost;
		MoneyMultiplySlow(&mult_price, order->Items[i].Item.Quantity);
		MoneySum(&total, &mult_price);
	}

	DEBUG("Charging card\n");
	char transaction_id[40];
	chargeCard(&desc, total, rr->req.CreditCard, transaction_id);

	shipOrder(&desc, &rr->req.address, order->Items,
		  rr->res.order.num_items, order->ShippingTrackingId);

	emptyUserCart(&desc, rr->req.UserId);

	DEBUG("Sending order confirmation\n");
	sendOrderConfirmation(&desc, rr->req.Email, order);

	unimsg_buffer_put(&desc, 1);

	DEBUG("Order placed\n");
}

static void handle_request(struct unimsg_shm_desc *desc)
{
	struct rpc *rpc = desc->addr;
	PlaceOrderRR *rr = (PlaceOrderRR *)rpc->rr;

	PlaceOrder(rr);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(CHECKOUT_SERVICE, handle_request, dependencies,
		    sizeof(dependencies) / sizeof(dependencies[0]));

	return 0;
}
