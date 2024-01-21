/*
 * Some sort of Copyright
 */

#include "../common/service/message.h"
#include "../common/service/service.h"

#define MAX_ADS_TO_SERVE 1

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
		DEBUG("No Ad found.\n");
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
			DEBUG("No Ad found.\n");
			Ad ad = {"", ""};
			return ad;
		}
	}

	DEBUG("No Ad found.\n");
	Ad ad = {"", ""};
	return ad;
}

static AdRequest* GetContextKeys(AdRR *rr) {
	return &(rr->req);
}

static void PrintContextKeys(AdRequest* ad_request) {
	int i;
	for(i = 0; i < ad_request->num_context_keys; i++) {
		DEBUG("context_word[%d]=%s\t\t", i + 1,
		      ad_request->ContextKeys[i]);
	}
	DEBUG("\n");
}

static void GetAds(AdRR *rr) {
	DEBUG("[GetAds] received ad request\n");

	AdRequest* ad_request = GetContextKeys(rr);
	PrintContextKeys(&rr->req);
	rr->res.num_ads = 0;

	if (ad_request->num_context_keys > 0) {
		DEBUG("Constructing Ads using received context.\n");
		int i;
		for(i = 0; i < ad_request->num_context_keys; i++) {
			DEBUG("context_word[%d]=%s\n", i + 1, ad_request->ContextKeys[i]);
			Ad ad = getAdsByCategory(ad_request->ContextKeys[i]);

			strcpy(rr->res.Ads[i].RedirectUrl, ad.RedirectUrl);
			strcpy(rr->res.Ads[i].Text, ad.Text);
			rr->res.num_ads++;
		}
	} else {
		DEBUG("No Context provided. Constructing random Ads.\n");
		Ad ad = getRandomAds();
		
		strcpy(rr->res.Ads[0].RedirectUrl, ad.RedirectUrl);
		strcpy(rr->res.Ads[0].Text, ad.Text);
		rr->res.num_ads++;
	}

	if (rr->res.num_ads == 0) {
		DEBUG("No Ads found based on context. Constructing random Ads.\n");
		Ad ad = getRandomAds();

		strcpy(rr->res.Ads[0].RedirectUrl, ad.RedirectUrl);
		strcpy(rr->res.Ads[0].Text, ad.Text);
		rr->res.num_ads++;
	}

	DEBUG("[GetAds] completed request\n");
}

static void handle_request(struct unimsg_shm_desc *descs,
			   unsigned *ndescs __unused)
{
	AdRR *rr = descs[0].addr;

	GetAds(rr);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	run_service(AD_SERVICE, handle_request);
	
	return 0;
}
