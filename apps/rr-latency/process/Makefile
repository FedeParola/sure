build/rr-latency: main.c build/redirect.bpf.o
	@mkdir -p $(@D)
	$(CC) -O2 -o $@ $< -lbpf

build/redirect.bpf.o: redirect.bpf.c
	@mkdir -p $(@D)
	clang -O2 -Wall -target bpf -g -c -o $@ $<