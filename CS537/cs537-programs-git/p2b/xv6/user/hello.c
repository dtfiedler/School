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
           printf(1,"Z\n");
	}
        int pid = fork();
	if(pid == 0){
		printf(1,"This is child\n");
		reserve(10);
		for(i=0;i<30;i++){
	           printf(1,"C\n");
       		 }
	}
	else{
		printf(1,"This is parent\n");
	}
	exit();
}
