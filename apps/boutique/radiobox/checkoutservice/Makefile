UK_ROOT ?= $(CURDIR)/../../../../unikraft
UK_LIBS ?= $(CURDIR)/../../../../libs
LIBS := $(UK_LIBS)/lib-unimsg:$(UK_LIBS)/lib-musl

all:
	@$(MAKE) -C $(UK_ROOT) A=$(CURDIR) L=$(LIBS) CFLAGS=$(CFLAGS)

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(CURDIR) L=$(LIBS) $(MAKECMDGOALS)
