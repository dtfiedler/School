#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) 
{
	int ch;
	printf("i am at here.");
	while ((ch = getopt(argc, argv, "i:o:")) != -1)
	{
		switch (ch) {
			case 'i': 
				printf(optarg);
				break;
			case 'o':
				printf(optarg);
				break;
			default: 
				printf("i dont know\n");	
		}
	}
	return 0;
}
