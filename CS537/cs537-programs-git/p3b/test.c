#include <stdio.h>

// test program in linux to dereferen a null ptr.
int main (int argc, char* argv[])
{
	int *x = 0;
	printf("*x is = %d\n", *x);
	return 0;
}
