#include <getopt.h>
#include <h2os/net.h>
#include <h2os/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>

static unsigned opt_iterations = 0;
static struct option long_options[] = {
	{"iterations", required_argument, 0, 'i'},
	{0, 0, 0, 0}
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"  Usage: %s [OPTIONS]\n"
		"  Options:\n"
		"  -i, --iterations	Number of requests-responses to exchange\n",
		prog);

	exit(1);
}

static void parse_command_line(int argc, char **argv)
{
	int option_index, c;

	for (;;) {
		c = getopt_long(argc, argv, "i:", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			opt_iterations = atoi(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (opt_iterations == 0) {
		fprintf(stderr, "Iterations is a required parameteres and must "
			"be > 0\n");
		usage(argv[0]);
	}
}

// static unsigned long global;
// static int __attribute__((noinline)) dummy()
// {
// 	global++;
// 	return 0;
// }

// static char stack[512];

// static inline __attribute__((always_inline)) int gate()
// {
// 	int rc = 0;

// 	uk_preempt_disable();

// 	// unsigned long tid = CONFIG_LIBH2OS_MAX_THREADS;
// 	struct uk_thread *_t = uk_thread_current();
// 	if (!_t)
// 		UK_CRASH("No thread");
// 	void *stackp = &stack[sizeof(stack)];
// 	asm volatile (
// 		/* Are we already in privileged mode? */
// 		"xor %%r12, %%r12\n\t"
// 		"xor %%rcx, %%rcx\n\t"
// 		"rdpkru\n\t"
// 		"cmp %[pkey], %%rax\n\t"
// 		"je 1f\n\t"
// 		"mov $1, %%r12\n\t"
// 		/* Set privileged PKRU */
// 		"xor %%rdx, %%rdx\n\t"
// 		"xor %%rcx, %%rcx\n\t"
// 		"mov %[pkey], %%rax\n\t"
// 		"wrpkru\n\t"
// 		/* Switch stack */
// 		/* Is the thread id valid? */
// 		// "cmp %[maxtid], %[tid]\n\t"
// 		// "jge 2f\n\t"
// 		// "shl $6, %[tid]\n\t"
// 		// "lea %[tinfo], %%rax\n\t"
// 		// "cmpq $0, (%%rax, %[tid])\n\t"
// 		// "je 2f\n\t"
// 		// "mov 0x8(%%rax, %[tid]), %%rdx\n\t"
// 		// "mov %%rsp, -0x8(%%rdx)\n\t"
// 		// "mov %%rdx, %%rsp\n\t"
// 		// "sub $0x8, %%rsp\n\t"
// 		"mov %%rsp, -0x8(%[stack])\n\t"
// 		"mov %[stack], %%rsp\n\t"
// 		"sub $0x8, %%rsp\n\t"
// 		"1:\n\t"

// 		"call dummy\n\t"

// 		/* Save call return code */
// 		"mov %%eax, %[rc]\n\t"
// 		/* Were we already in privileged mode? */
// 		"cmp $0, %%r12\n\t"
// 		"je 3f\n\t"
// 		/* Reset stack */
// 		"mov (%%rsp), %%rsp\n\t"
// 		/* Set default PKRU */
// 		"xor %%rdx, %%rdx\n\t"
// 		"xor %%rcx, %%rcx\n\t"
// 		"mov %[dkey], %%rax\n\t"
// 		"wrpkru\n\t"
// 		"cmp %[dkey], %%rax\n\t"
// 		"je 3f\n\t"
// 		"2:\n\t"
// 		"int $0xd\n\t" /* ROP detected, crash */
// 		"3:\n\t"
// 		: [rc]"=&r"(rc),
// 		  //[tid]"+r"(tid)
// 		  [stack]"+r"(stackp)
// 		: [pkey]"i"(H2OS_PKRU_PRIVILEGED),
// 		  [dkey]"i"(H2OS_PKRU_DEFAULT),
// 		  [maxtid]"i"(CONFIG_LIBH2OS_MAX_THREADS),
// 		  [tinfo]"m"(thread_infos)
// 		: "rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11",
// 		  "r12"
// 	);

// 	uk_preempt_enable();

// 	return rc;
// }

int main(int argc, char *argv[])
{
	parse_command_line(argc, argv);

	unsigned long start = ukplat_monotonic_clock();

	for (unsigned long i = 0; i < opt_iterations; i++)
		h2os_dummy();

	unsigned long stop = ukplat_monotonic_clock();

	printf("Total time: %lu ns, average call time: %lu ns\n",
		stop - start, (stop - start) / opt_iterations);

	return 0;
}