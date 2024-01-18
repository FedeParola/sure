// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

#include <c_lib.h>
#include <math.h>
#include "../common/service.h"
#include "../common/messages.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

char *product_ids[] = {"OLJCESPC7Z", "66VCHSJNUP", "1YMWWN1N4O", "L9ECAV7KIM", "2ZYFJ3GM2N", "0PUK6V6EV0", "LS4PSXUNUM", "9SIQT8TOJO", "6E92ZMYYFZ"};

Product products[9] = {
	{
		.Id = "OLJCESPC7Z",
		.Name = "Sunglasses",
		.Description = "Add a modern touch to your outfits with these sleek aviator sunglasses.",
		.Picture = "/static/img/products/sunglasses.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 19,
			.Nanos = 990000000
		},
		.num_categories = 1,
		.Categories = {"accessories"}
	},
	{
		.Id = "66VCHSJNUP",
		.Name = "Tank Top",
		.Description = "Perfectly cropped cotton tank, with a scooped neckline.",
		.Picture = "/static/img/products/tank-top.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 18,
			.Nanos = 990000000
		},
		.num_categories = 2,
		.Categories = {"clothing", "tops"}
	},
	{
		.Id = "1YMWWN1N4O",
		.Name = "Watch",
		.Description = "This gold-tone stainless steel watch will work with most of your outfits.",
		.Picture = "/static/img/products/watch.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 109,
			.Nanos = 990000000
		},
		.num_categories = 1,
		.Categories = {"accessories"}
	},
	{
		.Id = "L9ECAV7KIM",
		.Name = "Loafers",
		.Description = "A neat addition to your summer wardrobe.",
		.Picture = "/static/img/products/loafers.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 89,
			.Nanos = 990000000
		},
		.num_categories = 1,
		.Categories = {"footwear"}
	},
	{
		.Id = "2ZYFJ3GM2N",
		.Name = "Hairdryer",
		.Description = "This lightweight hairdryer has 3 heat and speed settings. It's perfect for travel.",
		.Picture = "/static/img/products/hairdryer.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 24,
			.Nanos = 990000000
		},
		.num_categories = 2,
		.Categories = {"hair", "beauty"}
	},
	{
		.Id = "0PUK6V6EV0",
		.Name = "Candle Holder",
		.Description = "This small but intricate candle holder is an excellent gift.",
		.Picture = "/static/img/products/candle-holder.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 18,
			.Nanos = 990000000
		},
		.num_categories = 2,
		.Categories = {"decor", "home"}
	},
	{
		.Id = "LS4PSXUNUM",
		.Name = "Salt & Pepper Shakers",
		.Description = "Add some flavor to your kitchen.",
		.Picture = "/static/img/products/salt-and-pepper-shakers.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 18,
			.Nanos = 490000000
		},
		.num_categories = 1,
		.Categories = {"kitchen"}
	},
	{
		.Id = "9SIQT8TOJO",
		.Name = "Bamboo Glass Jar",
		.Description = "This bamboo glass jar can hold 57 oz (1.7 l) and is perfect for any kitchen.",
		.Picture = "/static/img/products/bamboo-glass-jar.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 5,
			.Nanos = 490000000
		},
		.num_categories = 1,
		.Categories = {"kitchen"}
	},
	{
		.Id = "6E92ZMYYFZ",
		.Name = "Mug",
		.Description = "A simple mug with a mustard interior.",
		.Picture = "/static/img/products/mug.jpg",
		.PriceUsd = {
			.CurrencyCode = "USD",
			.Units = 8,
			.Nanos = 990000000
		},
		.num_categories = 1,
		.Categories = {"kitchen"}
	}
};

static int compare_e(void* left, void* right ) {
    return strcmp((const char *)left, (const char *)right);
}

struct clib_map* productcatalog_map;

static void parseCatalog(struct clib_map* map) {
    int size = sizeof(product_ids)/sizeof(product_ids[0]);
    int i = 0;
    for (i = 0; i < size; i++ ) {
        char *key = clib_strdup(product_ids[i]);
        int key_length = (int)strlen(key) + 1;
        Product value = products[i];
		printf("Inserting [%s -> %s]\n", key, value.Name);
        insert_c_map(map, key, key_length, &value, sizeof(Product)); 
        free(key);
    }
}

static void ListProducts(ListProductsResponse *out) {
	int size = sizeof(out->Products)/sizeof(out->Products[0]);
	out->num_products = 0;
	for (int i = 0; i < size; i++) {
		out->Products[i] = products[i];
		out->num_products++;
	}
	return;
}

static void GetProduct(GetProductRR *rr) {
	GetProductRequest *req = &rr->req;

	Product* found = &rr->res;
	int num_products = 0;
	
	int size = sizeof(product_ids)/sizeof(product_ids[0]);
	int i = 0;
	for (i = 0; i < size; i++ ) {
		if (strcmp(req->Id, product_ids[i]) == 0) {
			printf("Get Product: %s\n", product_ids[i]);
			num_products++;
			*found = products[i];
			break;
		}
	}

	if (num_products == 0) {
		printf("no product with ID %s\n", req->Id);
	}
	return;
}

static void SearchProducts(SearchProductsRR *rr) {
	SearchProductsRequest* req = &rr->req;
	SearchProductsResponse* out = &rr->res;
	out->num_products = 0;

	/* Intepret query as a substring match in name or description. */
	int size = sizeof(product_ids) / sizeof(product_ids[0]);
	int i = 0;
	for (i = 0; i < size; i++ ) {
		if (strstr(products[i].Name, req->Query) != NULL 
		    || strstr(products[i].Description, req->Query) != NULL ) {
			out->Results[out->num_products] = products[i];
			out->num_products++;
		}
	}
	return;
}

static void handle_request(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned nrecv;
	ProductCatalogRpc *rpc;

	nrecv = 1;
	int rc = unimsg_recv(s, &desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	rpc = desc.addr;

	switch (rpc->command) {
	case PRODUCT_CATALOG_COMMAND_LIST_PRODUCTS:
		ListProducts((ListProductsResponse *)&rpc->rr);
		break;
	case PRODUCT_CATALOG_COMMAND_GET_PRODUCT:
		GetProduct((GetProductRR *)&rpc->rr);
		break;
	case PRODUCT_CATALOG_COMMAND_SEARCH_PRODUCTS:
		SearchProducts((SearchProductsRR *)&rpc->rr);
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

	productcatalog_map = new_c_map(compare_e, NULL, NULL);
	parseCatalog(productcatalog_map);

	run_service(PRODUCTCATALOG_SERVICE, handle_request);
	
	return 0;
}
