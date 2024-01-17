SERVICES := adservice cartservice checkoutservice currencyservice	\
	    emailservice frontend paymentservice productcatalogservice	\
	    recommendationservice shippingservice

.PHONY: all $(SERVICES)

all: $(SERVICES)

$(SERVICES):
	$(info )
	$(info ==== Building $@ ====)
	@$(MAKE) -C $@ -j