ifdef ADDITIONAL_STATS
CFLAGS = "-D ADDITIONAL_STATS"
endif

build/throughput: main.c build/redirect.bpf.o
	@mkdir -p $(@D)
	$(CC) -O2 -o $@ $< -lbpf $(CFLAGS)

build/redirect.bpf.o: redirect.bpf.c
	@mkdir -p $(@D)
	clang -O2 -Wall -target bpf -g -c -o $@ $<

clean:
	rm -rf build