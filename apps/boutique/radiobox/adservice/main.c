/*
 * Some sort of Copyright
 */

#include "../common/service.h"
#include "../common/messages.h"

#define MAX_ADS_TO_SERVE 1
#define ERR_CLOSE(s) ({ unimsg_close(s); exit(1); })
#define ERR_PUT(descs, ndescs, s) ({					\
	unimsg_buffer_put(descs, ndescs);				\
	ERR_CLOSE(s);							\
})

char *ad_name[] = {"clothing", "accessories", "footwear", "hair", "decor", "kitchen"};

static Ad getAdsByCategory(char contextKey[]) {
	if (strcmp(contextKey, "clothing") == 0) {
		Ad ad = {"/product/66VCHSJNUP", "Tank top for sale. 20 off."};
		return ad;
	} else if (strcmp(contextKey, "accessories") == 0) {
		Ad ad = {"/product/1YMWWN1N4O", "Watch for sale. Buy one, get second kit for free"};
		return ad;
	} else if (strcmp(contextKey, "footwear") == 0) {
		Ad ad = {"/product/L9ECAV7KIM", "Loafers for sale. Buy one, get second one for free"};
		return ad;
	} else if (strcmp(contextKey, "hair") == 0) {
		Ad ad = {"/product/2ZYFJ3GM2N", "Hairdryer for sale. 50 off."};
		return ad;
	} else if (strcmp(contextKey, "decor") == 0) {
		Ad ad = {"/product/0PUK6V6EV0", "Candle holder for sale. 30 off."};
		return ad;
	} else if (strcmp(contextKey, "kitchen") == 0) {
		Ad ad = {"/product/6E92ZMYYFZ", "Mug for sale. Buy two, get third one for free"};
		return ad;
	} else {
		printf("No Ad found.\n");
		Ad ad = {"", ""};
		return ad;
	}
}

static Ad getRandomAds() {
	int i;
	int ad_index;

	for (i = 0; i < MAX_ADS_TO_SERVE; i++) {
		ad_index = rand() % 6;
		if (strcmp(ad_name[ad_index], "clothing") == 0) {
			Ad ad = {"/product/66VCHSJNUP", "Tank top for sale. 20 off."};
			return ad;
		} else if (strcmp(ad_name[ad_index], "accessories") == 0) {
			Ad ad = {"/product/1YMWWN1N4O", "Watch for sale. Buy one, get second kit for free"};
			return ad;
		} else if (strcmp(ad_name[ad_index], "footwear") == 0) {
			Ad ad = {"/product/L9ECAV7KIM", "Loafers for sale. Buy one, get second one for free"};
			return ad;
		} else if (strcmp(ad_name[ad_index], "hair") == 0) {
			Ad ad = {"/product/2ZYFJ3GM2N", "Hairdryer for sale. 50 off."};
			return ad;
		} else if (strcmp(ad_name[ad_index], "decor") == 0) {
			Ad ad = {"/product/0PUK6V6EV0", "Candle holder for sale. 30 off."};
			return ad;
		} else if (strcmp(ad_name[ad_index], "kitchen") == 0) {
			Ad ad = {"/product/6E92ZMYYFZ", "Mug for sale. Buy two, get third one for free"};
			return ad;
		} else {
			printf("No Ad found.\n");
			Ad ad = {"", ""};
			return ad;
		}
	}

	printf("No Ad found.\n");
	Ad ad = {"", ""};
	return ad;
}

static AdRequest* GetContextKeys(AdRR *rr) {
	return &(rr->req);
}

static void PrintContextKeys(AdRequest* ad_request) {
	int i;
	for(i = 0; i < ad_request->num_context_keys; i++) {
		printf("context_word[%d]=%s\t\t", i + 1,
		       ad_request->ContextKeys[i]);
	}
	printf("\n");
}

static void PrintAdResponse(AdRR *rr) {
	int i;
	printf("Ads in AdResponse:\n");
	for(i = 0; i < rr->res.num_ads; i++) {
		printf("Ad[%d] RedirectUrl: %s\tText: %s\n", i + 1,
		       rr->res.Ads[i].RedirectUrl, rr->res.Ads[i].Text);
	}
	printf("\n");
}

static void GetAds(AdRR *rr) {
	printf("[GetAds] received ad request\n");

	AdRequest* ad_request = GetContextKeys(rr);
	PrintContextKeys(&rr->req);
	rr->res.num_ads = 0;

	// []*pb.Ad allAds;
	if (ad_request->num_context_keys > 0) {
		printf("Constructing Ads using received context.\n");
		int i;
		for(i = 0; i < ad_request->num_context_keys; i++) {
			printf("context_word[%d]=%s\n", i + 1, ad_request->ContextKeys[i]);
			Ad ad = getAdsByCategory(ad_request->ContextKeys[i]);

			strcpy(rr->res.Ads[i].RedirectUrl, ad.RedirectUrl);
			strcpy(rr->res.Ads[i].Text, ad.Text);
			rr->res.num_ads++;
		}
	} else {
		printf("No Context provided. Constructing random Ads.\n");
		Ad ad = getRandomAds();
		
		strcpy(rr->res.Ads[0].RedirectUrl, ad.RedirectUrl);
		strcpy(rr->res.Ads[0].Text, ad.Text);
		rr->res.num_ads++;
	}

	if (rr->res.num_ads == 0) {
		printf("No Ads found based on context. Constructing random Ads.\n");
		Ad ad = getRandomAds();

		strcpy(rr->res.Ads[0].RedirectUrl, ad.RedirectUrl);
		strcpy(rr->res.Ads[0].Text, ad.Text);
		rr->res.num_ads++;
	}

	printf("[GetAds] completed request\n");
}

static void MockAdRequest(AdRR *rr) {
	int num_context_keys = 2;
	int i;
	
	rr->req.num_context_keys = 0;
	for (i = 0; i < num_context_keys; i++) {
		rr->req.num_context_keys++;
		strcpy(rr->req.ContextKeys[i], ad_name[i]);
	}
}

static void handle_request(struct unimsg_sock *s)
{
	struct unimsg_shm_desc desc;
	unsigned nrecv;
	AdRR *rr;

	nrecv = 1;
	int rc = unimsg_recv(s, &desc, &nrecv, 0);
	if (rc) {
		fprintf(stderr, "Error receiving desc: %s\n", strerror(-rc));
		ERR_CLOSE(s);
	}

	rr = desc.addr;

	GetAds(rr);

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

	run_service(AD_SERVICE, handle_request);
	
	return 0;
}
