#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(void) {
	int rv;
	int status = -1;
	rv = fork();
	if (rv == 0) {
		// c
		printf("child pid = %d\n", getpid());
		
		rv = fork();
		if (rv == 0) {
			// g
			printf("g pid = %d\n", getpid());
			
			rv = fork();
			if (rv == 0) {
				// gg
				 printf("gg pid = %d\n", getpid());
			}  else if (rv > 0) {
				// g
				printf("g pid = %d\n", getpid());
				wait(&status);
			}
		 } else if (rv > 0) {
        	        // c
			printf("child pid = %d\n", getpid());
			wait(&status);
	        }
	} else if (rv > 0) {
		// p
		printf("parent pid = %d\n", getpid());
		wait(&status);
		
	}
	return 0;
}
