#include "lib.h"

#include <stdio.h>

int main(void)
{
	puts("Started the test program.");
	fflush(stdout);

	printf("This will crash: %d.\n", data->x);

	puts("You shouldn't see this.");
	fflush(stdout);

	return 0;
}
