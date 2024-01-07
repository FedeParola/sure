ifdef ENABLE_SK_MSG
DEPS = build/redirect.bpf.o
CFLAGS = -lbpf -D ENABLE_SK_MSG
endif

ifdef ADDITIONAL_STATS
CFLAGS += -D ADDITIONAL_STATS
endif

build/rr-latency: main.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -O2 -o $@ $< $(CFLAGS)

build/redirect.bpf.o: redirect.bpf.c
	@mkdir -p $(@D)
	clang -O2 -Wall -target bpf -g -c -o $@ $<

clean:
	rm -rf build