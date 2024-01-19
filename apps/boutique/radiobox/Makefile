SERVICES := adservice cartservice checkoutservice currencyservice	\
	    emailservice frontend paymentservice productcatalogservice	\
	    recommendationservice shippingservice

.PHONY: all $(SERVICES) clean

all: $(SERVICES)

$(SERVICES):
	$(info )
	$(info ==== Building $@ ====)
	@$(MAKE) -C $@ -j

clean:
	for dir in $(SERVICES); do					\
		$(MAKE) -C $$dir clean;					\
        done