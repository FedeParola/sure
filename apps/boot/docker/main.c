#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

        struct timespec now;
        int rc = clock_gettime(CLOCK_MONOTONIC, &now);
        if (rc) {
                fprintf(stderr, "Error getting time: %s\n", strerror(rc));
                exit(EXIT_FAILURE);
        }

        printf("%lu\n", now.tv_nsec + now.tv_sec * 1000000000);

	return 0;
}
