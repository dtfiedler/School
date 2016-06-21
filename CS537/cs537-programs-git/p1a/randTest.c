#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int args, char *argv[])
{
	int i;
	/*
	printf("not use seed\n");
	for(i = 0; i < 15; i++)
	{
		printf("%d\n", rand());
	}
	*/

	printf("use seed\n");
	srand((unsigned)time(NULL));
	for (i = 0; i<15; i++)
	{
		printf("%d\n", rand()%10);
	}
	return 0;
}
