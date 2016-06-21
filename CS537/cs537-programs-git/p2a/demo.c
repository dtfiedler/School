#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main (int argc, char* argv[])
{
	printf("I am parent\n");
	int rv;
	rv = fork();
	if (rv == 0) {
		printf("I am child, pid= %d, rv = %d \n", getpid(), rv);
		
	} else if (rv >= 0) {
		printf("I am parent, pid= %d, rv = %d \n", getpid(), rv);
	}
	return 0;
}
