UK_ROOT ?= $(PWD)/../../../unikraft
UK_LIBS ?= $(PWD)/../../../libs
LIBS := $(UK_LIBS)/lib-unimsg

all:
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS)

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
