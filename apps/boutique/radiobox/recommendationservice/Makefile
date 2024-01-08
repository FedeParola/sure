UK_ROOT ?= $(PWD)/../../../../unikraft
UK_LIBS ?= $(PWD)/../../../../libs
LIBS := $(UK_LIBS)/lib-unimsg:$(UK_LIBS)/lib-musl

all:
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) CFLAGS=$(CFLAGS)

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
