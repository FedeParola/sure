// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2022 University of California, Riverside
 */

// #include <math.h>
#include "../common/service/service.h"

#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

static int dependencies[] = {
	PRODUCTCATALOG_SERVICE,
};
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

// ListRecommendations fetch list of products from product catalog stub
static void ListRecommendations(ListRecommendationsRR *rr)
{
	int rc;

	DEBUG("[ListRecommendations] received request\n");

	struct unimsg_shm_desc desc;
	rc = unimsg_buffer_get(&desc, 1);
	if (rc) {
		fprintf(stderr, "Error getting shm buffer: %s\n",
			strerror(-rc));
		exit(1);
	}

	struct rpc *rpc = desc.addr;
	rpc->command = PRODUCTCATALOG_LIST_PRODUCTS;
	desc.size = sizeof(*rpc) + sizeof(ListProductsResponse);

	do_rpc(&desc, PRODUCTCATALOG_SERVICE, sizeof(ListProductsResponse));

	rpc = desc.addr;
	ListProductsResponse *list_products_response =
		(ListProductsResponse *)rpc->rr;
	ListRecommendationsRequest *list_recommendations_request = &rr->req;
	ListRecommendationsResponse *out = &rr->res;

	// 1. Filter products
	strcpy(out->product_ids[0],
	       list_recommendations_request->product_ids[0]);

	// 2. sample list of indicies to return
	int product_list_size = sizeof(list_products_response->Products)
				/ sizeof(list_products_response->Products[0]);
	int recommended_product = rand() % product_list_size;

	// 3. Generate a response.
	strcpy(out->product_ids[0], products[recommended_product].Id);

	unimsg_buffer_put(&desc, 1);

	return;
}

static void handle_request(struct unimsg_shm_desc *descs,
			   unsigned *ndescs __unused)
{
	struct rpc *rpc = descs[0].addr;
	ListRecommendationsRR *rr = (ListRecommendationsRR *)rpc->rr;

	ListRecommendations(rr);
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

		rc = unimsg_connect(socks[id], services[id].addr,
				    services[id].port);
		if (rc) {
			fprintf(stderr, "Error connecting to %s service: %s\n",
				services[id].name, strerror(-rc));
			return 1;
		}

		DEBUG("Connected to %s service\n", services[id].name);
	}

	run_service(RECOMMENDATION_SERVICE, handle_request);

	return 0;
}