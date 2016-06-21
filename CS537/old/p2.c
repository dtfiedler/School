#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define BUFFERSIZE 128
#define BIG_BUFFERSIZE 1024
#define BUILTINCOUNT 4
char error_message[30] = "An error has occurred\n";


int pwdFunction(char *myargv[]);
int cdFunction(char *myargv[]);
int pathFunction(char *myargv[]);
int exitFunction(char *myargv[]);


char *builtIn[] = {
	"pwd",
	"cd",
	"path",
	"exit"
};

char **paths;

int (*builtInFunctions[]) (char **) = {
	&pwdFunction,
	&cdFunction,
	&pathFunction,
	&exitFunction
};

int pwdFunction(char *myargv[]){
	char *buffer = (char*)malloc(sizeof(char));
	//write(STDOUT_FILENO, "whoosh> ", strlen("whoosh> "));
	getcwd(buffer, BUFFERSIZE);
	strncat(buffer, "\n", 1);
	write(STDOUT_FILENO, buffer, strlen(buffer));
	return 1;
	//doesn't seem correct
}

int exitFunction(char *myargv[]){
	exit(0);
}

//path builtin
int pathFunction(char *myargv[]){
	paths = (char**)malloc(sizeof(*myargv));
	int i = 1;
	while(myargv[i] != NULL){
		paths[i - 1] = (char*)malloc(1 + strlen(myargv[i]));
		strncpy(paths[i - 1], myargv[i], strlen(myargv[i]) + 1);
		i++;
	}
	return 1;
}

//cd builtin
int cdFunction(char *myargv[]){
	int ret;
	if (myargv[1] == NULL){
		char *buffer = getenv("HOME");
		ret = chdir(buffer);
		strncat(buffer, "\n", 1);
	} else {
		 char *directoryPath = (char*)malloc(strlen(myargv[1]));
		 strncpy(directoryPath, myargv[1], strlen(myargv[1]) - 1);
		 ret = chdir(directoryPath);
		 strncat(directoryPath, "\n", 1);
	}

	if (ret != 0){
		write(STDERR_FILENO, error_message, strlen(error_message));
	}

	return 1;
}

int alternateProgram(char *myargv[]){
		int rc = fork();
		if(rc == 0){
			int i = 0; 
			char *defaultPath = (char*)malloc(sizeof(BUFFERSIZE));
			strncpy(defaultPath, "/bin/", strlen("/bin/"));
			if (strncmp(myargv[0], "ls", strlen(myargv[0]) - 1) == 0){
				strncat(defaultPath, myargv[0], strlen(myargv[0]) - 1);
			} else {
				strncat(defaultPath, myargv[0], strlen(myargv[0]));
			}
			char *current = (char*)malloc(sizeof(char));
			if (strncmp(myargv[1], "-ls", strlen(myargv[1]) - 1) == 0){
				printf("%s\n", "got it");
			}
			
			execv(defaultPath, myargv);
			free(defaultPath);
			if (paths == NULL){
				//fprintf(stderr, "%s", error_message );
			} else {
				while(paths[i] != NULL){
					char *executeString = (char*)malloc(1 + strlen(paths[i]) + strlen(myargv[0]));
					strncpy(executeString, paths[i], strlen(paths[i]) - 1);
					//strncat(executeString, "/", 1);
					strncat(executeString, myargv[0], strlen(myargv[0]) - 1);
					//close(STDOUT_FILENO);
					// open("/tmp/file.stdout", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	  		 		execv(executeString, myargv);
					i++;
				}
			 }
		} else if (rc > 0){
			//parent
			int cpid = (int) wait(NULL);
		} else {
				fprintf(stderr, "fail\n");
		}
	return 1;
}

//first check if command is builtin program, otherwise, 
//perform execv funciton
int launchProgram(char *myargv[]){
	char **path = (char**) malloc(sizeof(*myargv));
    // //new process created, new virtual address space!
	//child
	int i;
	for (i = 0; i < BUILTINCOUNT; i++){
		if (strncmp(builtIn[i], myargv[0], strlen(myargv[0]) - 1) == 0){
			return builtInFunctions[i](myargv);
		}
	}
	return alternateProgram(myargv);
	//printf("%s\n", "here1");
}

//write(STOUT,FILENO, "Test strings\n", strlen("Test strings"));

/*gets arguments from command line and performs built in funcitons, 
or creates new child processes based on the determined path*/
int executeShell(char *argv[]){
	//char error_message[30] = "An error has occurred\n";
	char **myargv = (char**)malloc(sizeof(char));
	if (myargv == NULL){
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	char buffer[500];
	int argumentCount = 0;
	fgets(buffer, 500, stdin);
	if (strlen(buffer) - 1 > BUFFERSIZE){
		return 1;
	} else {
		//printf("%lu\n", strlen(buffer));
		char *cmd = strtok(buffer, " ");
		while (cmd != NULL){
	          myargv[argumentCount] = (char*) malloc (sizeof(char) * (strlen(cmd) - 1));
	          if (myargv[argumentCount] == NULL){
	          	fprintf(stderr, "malloc failed\n");
				exit(1);
	          }
	          strncpy(myargv[argumentCount], cmd, strlen(cmd) - 1);
	          argumentCount++;
	          cmd = strtok(NULL, " ");
	      }
	   for (int i = 0; i < argumentCount ; i++){
				printf("%s", myargv[i]);
		}
	    myargv[argumentCount] = NULL;//NULL;//important to clarify end of arguments
		return launchProgram(myargv);   	
    }
	return 1;
}

void shellLoop(){

	int status;
	char **args;
 	do {
 		write(STDOUT_FILENO, "whoosh> ", strlen("whoosh> "));
   	 	status = executeShell(args);
    } while (status);
}

int main (int argc, char *argv[]){
	shellLoop();
	return 0;
}

