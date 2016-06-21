#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LENGTH 1024

int main (int argc, char *argv[]) {
	while(1) {
		printf("mysh> ");
		// line stores the contents from the commnad line. 
		
		char line [MAX_LENGTH];
		// "char array" , after read from stdin, 'enter' -> \n, and '\0'automatically append to the end of the input, 
		assert(fgets(line, MAX_LENGTH, stdin) != NULL);
		char *ptr;
		if ((ptr = strchr(line, '\n')) != NULL) {
			*ptr = '\0';
		}
		
		// the code below tries to parase the c string 'line' to an array
		int i;
		char* token, *str; // why *token
		char* saveptr;
		char* argv2 [MAX_LENGTH];
		
		for (i = 0, str=line; ;i++, str=NULL) {
			token = strtok_r(str, " ", &saveptr); // " " - the second para. indicates the content to used to parase the str, "/ " - finds "/" and " " and sepearte do t			the parase. 
			argv2[i] = token; //argv2 stores the content of the paraseed content. 
			if (token == NULL)	
				break;
		}		
	
		// fork the child process to run the require task. 
		int rv; 
		rv = fork();
		if (rv == 0) {
			// printf("Child\n");
			if (argv2[0] == NULL)
				break;
			// use child process to run the require command. 
			execvp(argv2[0], argv2);
		} else if (rv >= 0) {
			// printf("parent\n");
			wait(NULL);
		}		
	}
	return 0;		
}
