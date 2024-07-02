#include <stdio.h>
#include <uk/plat/common/cpu.h>

#define GUEST_EXIT_PORT  0xf4
#define GUEST_START_USER 0x7

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	outb(GUEST_EXIT_PORT, GUEST_START_USER);

	printf("Hello world!\n");

	return 0;
}
