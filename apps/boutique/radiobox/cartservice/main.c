// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <c_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unimsg/net.h>
#include "../common/service/service_sync.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static int compare_e(void* left, void* right ) {
    return strcmp((const char *)left, (const char *)right);
}

struct clib_map* LocalCartStore;

static void PrintUserCart(Cart *cart) {
	DEBUG("Cart for user %s: \n", cart->UserId);
	DEBUG("## %d items in the cart: ", cart->num_items);
	int i;
	for (i = 0; i < cart->num_items; i++) {
		DEBUG("\t%d. ProductId: %s \tQuantity: %d\n", i + 1, cart->Items[i].ProductId, cart->Items[i].Quantity);
	}
	DEBUG("\n");
	return;
}

static void AddItemAsync(char *userId, char *productId, int32_t quantity) {
	DEBUG("AddItemAsync called with userId=%s, productId=%s, quantity=%d\n", userId, productId, quantity);

	Cart newCart = {
		.UserId = "",
		.Items = {
			{
				.ProductId = "",
				.Quantity = quantity
			}
		}
	};

	strcpy(newCart.UserId, userId);
	strcpy(newCart.Items[0].ProductId, productId);

	void* cart;
	if (!find_c_map(LocalCartStore, userId, &cart)) {
		DEBUG("Add new carts for user %s\n", userId);
		char *key = clib_strdup(userId);
		int key_length = (int)strlen(key) + 1;
		newCart.num_items = 1;
		DEBUG("Inserting [%s -> %s]\n", key, newCart.UserId);
		insert_c_map(LocalCartStore, key, key_length, &newCart, sizeof(Cart));
		free(key);
	} else {
		DEBUG("Found carts for user %s\n", userId);
		int cnt = 0;
		int i;
		for (i = 0; i < ((Cart*)cart)->num_items; i++) {
			if (strcmp(((Cart*)cart)->Items[i].ProductId, productId) == 0) { // If the item exists, we update its quantity
				DEBUG("Update carts for user %s - the item exists, we update its quantity\n", userId);
				((Cart*)cart)->Items[i].Quantity++;
			} else {
				cnt++;
			}
		}

		if (cnt == ((Cart*)cart)->num_items) { // The item doesn't exist, we update it into DB
			DEBUG("Update carts for user %s - The item doesn't exist, we update it into DB\n", userId);
			((Cart*)cart)->num_items++;
			strcpy(((Cart*)cart)->Items[((Cart*)cart)->num_items].ProductId, productId);
			((Cart*)cart)->Items[((Cart*)cart)->num_items].Quantity = quantity;
		}

		free(cart);
	}
	return;
}

static void AddItem(AddItemRequest *in) {
	DEBUG("[AddItem] received request\n");

	AddItemAsync(in->UserId, in->Item.ProductId, in->Item.Quantity);
	return;
}

static void GetCartAsync(GetCartRR *rr) {
	GetCartRequest *in = &rr->req;
	Cart *out = &rr->res;
	DEBUG("[GetCart] GetCartAsync called with userId=%s\n", in->UserId);

	void *cart;
	if (!find_c_map(LocalCartStore, in->UserId, &cart)) {
		DEBUG("No carts for user %s\n", in->UserId);
		out->num_items = 0;
		return;
	} else {
		*out = *(Cart*)cart;
		free(cart);
		return;
	}
}

static void GetCart(GetCartRR *rr){
	GetCartAsync(rr);
	return;
}

static void EmptyCartAsync(EmptyCartRequest *in) {
	DEBUG("EmptyCartAsync called with userId=%s\n", in->UserId);

	void *cart;
	if (!find_c_map(LocalCartStore, in->UserId, &cart)) {
		DEBUG("No carts for user %s\n", in->UserId);
		// out->num_items = -1;
		return;
	} else {
		int i;
		for (i = 0; i < ((Cart*)cart)->num_items; i++) {
			DEBUG("Clean up item %d\n", i + 1);
			strcpy((*((Cart**)(&cart)))->Items[i].ProductId, "");
			((*((Cart**)(&cart))))->Items[i].Quantity = 0;
		}
		PrintUserCart((Cart*)cart);
		free(cart);
		return;
	}
}

static void EmptyCart(EmptyCartRequest *in) {
	DEBUG("[EmptyCart] received request\n");
	EmptyCartAsync(in);
	return;
}

static void handle_request(struct unimsg_shm_desc *descs,
			   unsigned *ndescs __unused)
{
	struct rpc *rpc = descs[0].addr;

	switch (rpc->command) {
	case CART_ADD_ITEM:
		AddItem((AddItemRequest *)rpc->rr);
		break;
	case CART_GET_CART:
		GetCart((GetCartRR *)rpc->rr);
		break;
	case CART_EMPTY_CART:
		EmptyCart((EmptyCartRequest *)rpc->rr);
		break;
	default:
		fprintf(stderr, "Received unknown command\n");
	}
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	LocalCartStore = new_c_map(compare_e, NULL, NULL);

	run_service(CART_SERVICE, handle_request);

	return 0;
}
