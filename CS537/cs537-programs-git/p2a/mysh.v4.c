#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LENGTH 1024
#define PIPE_LENGTH 1

int main (int argc, char *argv[]) {
	while(1) {
		printf("mysh> ");
		// line stores the contents from the commnad line. 
			
		char line [MAX_LENGTH];
		// "char array" , after read from stdin, 'enter' -> \n, and '\0'automatically append to the end of the input, 
		assert(fgets(line, MAX_LENGTH, stdin) != NULL);
		//char *ptr;
		//if ((ptr = strchr(line, '\n')) != NULL) {
		//	*ptr = '\0';
		//}
		// the code below tries to parase the c string 'line' to an array
		int i;
		int status = -1; // $status = -1 wait for any child return
		int ret = -1; // return value for pipe
		int fd[2] = {0}; // ? int array ?
		char* token, *str; // why *token
		char* saveptr;
		char* argv2 [MAX_LENGTH];
		int redirect = 0; // 0 -> no, 1 -> yes
		int go_pipe = 0; // 0 -> no pipe, 1 -> go pipe  
		int redirName; // index of array that points to the name of the file to be stored
		int truncate = 0; // 1 -> when truncate
		int append = 0; // 1 -> when append
		int pipe_index = 0; // create a char ptr array to store the index of the '|' sign. 
		for (i = 0, str=line; ;i++, str= NULL) {
			token = strtok_r(str, " \n", &saveptr); // " " - the second para. indicates the content to used to parase the str, "/ " - finds "/" and " " and sepearte do t			the parase. 
			argv2[i] = token; //argv2 stores the content of the paraseed content. 
			if (token == NULL)
				break;
			// check overwrite direction
			if (strcmp(token, ">") == 0) {
				redirect = 1;
				redirName = i + 1; // get index of file name  after '>'
				truncate = 1; // set truncate bit
			}
			if (strcmp(token, ">>") == 0) {
                                redirect = 1;
                                redirName = i + 1; // get index of file name after '>>'
                                append = 1; // set append bit
                        }
			// check pipe
			if (strcmp(token, "|") == 0) {
                                go_pipe = 1;
                                pipe_index = i; // get pipe sign index
                        }

			
		}

		// if command line is empty
		if (argv2[0] == NULL) {
                	continue;
		}
		// if command 'exit', quit mysh
		if (strcmp(argv2[0], "exit") == 0) {
			// if the exit follow with args, return Error!
			if (argv2[1] != NULL){
				fprintf(stderr, "Error!\n");
				continue;
			}
			// if the command line with 'exit' is not follow by any args, exit.
			exit(0);
		}
		 
		if (strcmp(argv2[0], "cd") == 0) {
			if (argv2[1] == NULL) {
				if(chdir(getenv("HOME")) == -1)
					fprintf(stderr, "Error!\n");
			} else {
				if(chdir(argv2[1]) == -1) {
					fprintf(stderr, "Error!\n");
				}
			}
			continue;	
		}
		// pwd 
		if (strcmp(argv2[0], "pwd") == 0) {
                        char cwd [1024];
			assert(getcwd(cwd, 1024) != NULL);
			printf("%s\n", cwd);
			continue;
                }

		// fork the child process to run the require task. 
		int rv; 
		rv = fork();
		if (rv == 0) {
			// printf("Child\n");
			if (argv2[0] == NULL) {
				break;
			}
			if (redirect) {
				close(1); // close STDOUT
				int fd; 
				if (truncate) {
					fd = open(argv2[redirName], O_CREAT|O_WRONLY|O_TRUNC, 0666);
					argv2[redirName - 1] = NULL; // reset the NULL pointer for the argv2
					// printf("redirName: %s, %s\n", argv2[redirName], argv2[redirName + 1]);
				}
				if (append) {
					fd = open(argv2[redirName], O_CREAT|O_WRONLY|O_APPEND, 0666);
                			argv2[redirName - 1] = NULL; // reset the NULL pointer for the argv2
				}
			}
				
			if (go_pipe) {
				ret = pipe(fd);
				if (ret == -1) {
					perror("Pipe failed!\n");
					// goto _ERR;
				}
				rv = fork(); // grandchild
				if (rv == 0) {
					// grandchild process
					close(fd[0]); // close read pipe
					dup2(fd[1], 1); // cp fd[1] as the STDOUT
					argv2[pipe_index] = NULL;
					if (execvp(argv2[0], argv2) == -1)
						perror("Error!\n");
				}
				if (rv > 0) {
					// child process
					wait(&status); // child wait for grandchild
					close(fd[1]); // close write pipe
					dup2(fd[0], 0); // cp fd[0] as the STDIN
					//argv2 = argv2[pipe_index + 1];
					//argv2+(pipe_index+1)
					if (execvp((argv2+(pipe_index+1))[0], argv2+(pipe_index+1)) == -1) 
						perror("ERROR!\n");
				}
			}
					
	
            
			// use child process to run the require command. 
			if(execvp(argv2[0], argv2) == -1)
				perror("Error!\n");
		} else if (rv > 0) {
			// printf("parent\n");
			wait(&status); // parent wait for child
		}		
	}
	return 0;		
}

