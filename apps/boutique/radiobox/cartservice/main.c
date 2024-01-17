// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <c_lib.h>
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

static int compare_e(void* left, void* right ) {
    return strcmp((const char *)left, (const char *)right);
}

struct clib_map* LocalCartStore;

static void PrintUserCart(Cart *cart) {
	printf("Cart for user %s: \n", cart->UserId);
	printf("## %d items in the cart: ", cart->num_items);
	int i;
	for (i = 0; i < cart->num_items; i++) {
		printf("\t%d. ProductId: %s \tQuantity: %d\n", i + 1, cart->Items[i].ProductId, cart->Items[i].Quantity);
	}
	printf("\n");
	return;
}

static void PrintLocalCartStore() {
	printf("\t\t #### PrintLocalCartStore ####\n");

	struct clib_iterator *myItr;
	struct clib_object *pElement;
	myItr = new_iterator_c_map(LocalCartStore);
	pElement = myItr->get_next(myItr);

	while (pElement) {
		void* cart = myItr->get_value(pElement);
		PrintUserCart((Cart*)cart);
		free(cart);
		pElement = myItr->get_next(myItr);
	}
	delete_iterator_c_map(myItr);
	printf("\n");
}

static void AddItemAsync(char *userId, char *productId, int32_t quantity) {
	printf("AddItemAsync called with userId=%s, productId=%s, quantity=%d\n", userId, productId, quantity);

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
	if (clib_true != find_c_map(LocalCartStore, userId, &cart)) {
		printf("Add new carts for user %s\n", userId);
		char *key = clib_strdup(userId);
		int key_length = (int)strlen(key) + 1;
		newCart.num_items = 1;
		printf("Inserting [%s -> %s]\n", key, newCart.UserId);
		insert_c_map(LocalCartStore, key, key_length, &newCart, sizeof(Cart));
		free(key);
	} else {
		printf("Found carts for user %s\n", userId);
		int cnt = 0;
		int i;
		for (i = 0; i < ((Cart*)cart)->num_items; i++) {
			if (strcmp(((Cart*)cart)->Items[i].ProductId, productId) == 0) { // If the item exists, we update its quantity
				printf("Update carts for user %s - the item exists, we update its quantity\n", userId);
				((Cart*)cart)->Items[i].Quantity++;
			} else {
				cnt++;
			}
		}

		if (cnt == ((Cart*)cart)->num_items) { // The item doesn't exist, we update it into DB
			printf("Update carts for user %s - The item doesn't exist, we update it into DB\n", userId);
			((Cart*)cart)->num_items++;
			strcpy(((Cart*)cart)->Items[((Cart*)cart)->num_items].ProductId, productId);
			((Cart*)cart)->Items[((Cart*)cart)->num_items].Quantity = quantity;
		}
	}
	return;
}

static void MockAddItemRequest(AddItemRequest *in) {
	strcpy(in->UserId, "spright-online-boutique");
	strcpy(in->Item.ProductId, "OLJCESPC7Z");
	in->Item.Quantity = 5;
	return;
}

static void AddItem(AddItemRequest *in) {
	printf("[AddItem] received request\n");

	AddItemAsync(in->UserId, in->Item.ProductId, in->Item.Quantity);
	return;
}

static void GetCartAsync(GetCartRR *rr) {
	GetCartRequest *in = &rr->req;
	Cart *out = &rr->res;
	printf("[GetCart] GetCartAsync called with userId=%s\n", in->UserId);

	void *cart;
	if (clib_true != find_c_map(LocalCartStore, in->UserId, &cart)) {
		printf("No carts for user %s\n", in->UserId);
		out->num_items = 0;
		return;
	} else {
		*out = *(Cart*)cart;
		return;
	}
}

static void GetCart(GetCartRR *rr){
	GetCartAsync(rr);
	return;
}

static void MockGetCartRequest(GetCartRR *rr) {
	GetCartRequest *in = &rr->req;
	strcpy(in->UserId, "spright-online-boutique");
	return;
}

static void PrintGetCartResponse(GetCartRR *rr) {
	printf("\t\t#### PrintGetCartResponse ####\n");
	Cart *out = &rr->res;
	printf("Cart for user %s: \n", out->UserId);
	int i;
	for (i = 0; i < out->num_items; i++) {
		printf("\t%d. ProductId: %s \tQuantity: %d\n", i + 1, out->Items[i].ProductId, out->Items[i].Quantity);
	}
	printf("\n");
	return;
}

static void EmptyCartAsync(EmptyCartRequest *in) {
	printf("EmptyCartAsync called with userId=%s\n", in->UserId);

	void *cart;
	if (clib_true != find_c_map(LocalCartStore, in->UserId, &cart)) {
		printf("No carts for user %s\n", in->UserId);
		// out->num_items = -1;
		return;
	} else {
		int i;
		for (i = 0; i < ((Cart*)cart)->num_items; i++) {
			printf("Clean up item %d\n", i + 1);
			strcpy((*((Cart**)(&cart)))->Items[i].ProductId, "");
			((*((Cart**)(&cart))))->Items[i].Quantity = 0;
		}
		PrintUserCart((Cart*)cart);
		return;
	}
}

static void EmptyCart(EmptyCartRequest *in) {
	printf("[EmptyCart] received request\n");
	EmptyCartAsync(in);
	return;
}

static void MockEmptyCartRequest(EmptyCartRequest *in) {
	strcpy(in->UserId, "spright-online-boutique");
}

int main(int argc, char **argv)
{
	int rc;
	struct unimsg_sock *s;

	(void)argc;
	(void)argv;

	LocalCartStore = new_c_map(compare_e, NULL, NULL);

	rc = unimsg_socket(&s);
	if (rc) {
		fprintf(stderr, "Error creating unimsg socket: %s\n",
			strerror(-rc));
		return 1;
	}

	rc = unimsg_bind(s, services[CART_SERVICE].port);
	if (rc) {
		fprintf(stderr, "Error binding to port %d: %s\n",
			services[CART_SERVICE].port, strerror(-rc));
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
		CartRpc *rpc;

		nrecv = 1;
		rc = unimsg_recv(cs, &desc, &nrecv, 0);
		if (rc) {
			fprintf(stderr, "Error receiving desc: %s\n",
				strerror(-rc));
			ERR_CLOSE(s);
		}

		rpc = desc.addr;

		switch (rpc->command) {
		case CART_COMMAND_ADD_ITEM:
			AddItem((AddItemRequest *)&rpc->rr);
			break;
		case CART_COMMAND_GET_CART:
			GetCart((GetCartRR *)&rpc->rr);
			break;
		case CART_COMMAND_EMPTY_CART:
			EmptyCart((EmptyCartRequest *)&rpc->rr);
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
