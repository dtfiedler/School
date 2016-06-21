#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argac, char *argv [])
{
	printf(1,"return value is %d\n",reserve(10));
	int i;
	for(i=0;i<30;i++){
           printf(1,"Q\n");
	}
	exit();
}
