#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

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
int output;

int (*builtInFunctions[]) (char **) = {
	&pwdFunction,
	&cdFunction,
	&pathFunction,
	&exitFunction
};

int pwdFunction(char *myargv[]){
	char *buffer = (char*)malloc(sizeof(char));
	getcwd(buffer, BUFFERSIZE);
	strncat(buffer, "\n", 1);
	write(STDOUT_FILENO, buffer, strlen(buffer));
	return 1;
}

int exitFunction(char *myargv[]){
	fflush(stdout);
	exit(0);
	exit(0);
	exit(0);
	return 0;
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
	} else {
		 char *directoryPath = (char*)malloc(strlen(myargv[1]));
		 strncpy(directoryPath, myargv[1], strlen(myargv[1]));

		 ret = chdir(directoryPath);
		 strncat(directoryPath, "\n", 1);
	}

	if (ret != 0){
		write(STDERR_FILENO, error_message, strlen(error_message));
	}

	return 1;
}
void outputFile(char *outputPath){


}
int alternateProgram(char *myargv[]){
		int rc = fork();
		int error;
		int fileNumber;
		char *buffer = (char*)malloc(BUFFERSIZE);
		if(rc == 0){
			int i = 0; 
			int y = 0;
			char *defaultPath = (char*)malloc(BUFFERSIZE);
			strncpy(defaultPath, "/bin/", strlen("/bin/"));
			strncat(defaultPath, myargv[0], strlen(myargv[0]));
		    if (output == 1) {
		    		char *mynewargv[4];
		    		mynewargv[0] = strdup(defaultPath);
		    		i = 1;
		    		while (strcmp(myargv[i], ">") != 0){
		    			mynewargv[i] = strdup(myargv[i]);
		    			i++;
		    		}
		    		if (myargv[i + 1] == NULL || myargv[i + 2] != NULL || *myargv[i + 1 ] == '/'){
		    			write(STDERR_FILENO, error_message, strlen(error_message));
		    			exit(0);
		    		}
					mynewargv[i] = NULL;//necessary

					//close stdin and stdout
					
					char *buffer = (char*)malloc(BUFFERSIZE);
					char *errorFile = (char*)malloc(strlen(buffer) + 4);
					struct stat st;
					getcwd(buffer, BUFFERSIZE);

					strncat(buffer, "/", 1);
					strncat(buffer, strdup(myargv[i + 1]), strlen(myargv[i + 1]));


					close(STDERR_FILENO);
					close(STDOUT_FILENO);
		   			strncpy(errorFile, buffer, strlen(buffer));
		   			strncat(errorFile, ".err", strlen(".err"));
		   			strncat(buffer, ".out", 4);

		   			//set new file paths
					open(buffer, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    				open(errorFile, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    				
    				execv(defaultPath, mynewargv);
    				write(STDERR_FILENO, error_message, strlen(error_message));
    				exit(0);
             }
            char *firstArg = strdup(myargv[0]);
			myargv[0] = strdup(defaultPath);
			execv(defaultPath, myargv);
			if (paths == NULL){
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(0);
			} else {
				i = 0;
				while(paths[i] != NULL){
					char *executeString = (char*)malloc(1 + sizeof(BUFFERSIZE));
					strncpy(executeString, paths[i], strlen(paths[i]));
					strncat(executeString, "/", 1);
					strncat(executeString, firstArg, strlen(myargv[0]));
					error = execv(executeString, myargv);
					i++;
				}
			 }
			 exit(0);
		} else if (rc > 0){
			//parent
			int cpid = (int) wait(NULL);
			if (output == 1){
				close(fileNumber);
			}
		} else {
				write(STDERR_FILENO, error_message, strlen(error_message));
		}
	return 1;
}

//first check if command is builtin program, otherwise, 
//perform execv funciton
int launchProgram(char *myargv[]){
        // //new process created, new virtual address space!
	//child
	fflush(stdin);
	int i;
	for (i = 0; i < BUILTINCOUNT; i++){
		if (strncmp(builtIn[i], myargv[0], strlen(myargv[0])) == 0){
			int compare = strncmp(builtIn[i], myargv[0], strlen(myargv[0]));
			return builtInFunctions[i](myargv);
		}
	}
	return alternateProgram(myargv);
}


/*gets arguments from command line and performs built in funcitons, 
or creates new child processes based on the determined path*/
int executeShell(char *argv[]){
	//char error_message[30] = "An error has occurred\n";
	char **myargv = (char**)malloc(BUFFERSIZE);
	if (myargv == NULL){
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	char buffer[500];
	int argumentCount = 0;
	fgets(buffer, 1000, stdin); 
	if (strlen(buffer) - 1 > BUFFERSIZE){
				write(STDERR_FILENO, error_message, strlen(error_message));
				return 1;
	}
	if (strncmp(buffer, "\0", 2) == 0 || strncmp(buffer, "\n", 2) == 0)  {
		return 1;
	} else {
		char *cmd = strtok(buffer, " \n");
		while (cmd != NULL){
	          myargv[argumentCount] = (char*) malloc (sizeof(char) * (strlen(cmd)));
	          if (myargv[argumentCount] == NULL){
	  			write(STDERR_FILENO, error_message, strlen(error_message));
				exit(1);
	          }
	          if (strcmp(cmd, "\0") != 0){
	          	if (strcmp(cmd, ">") == 0){
				output = 1;
			}
			 strncpy(myargv[argumentCount], cmd, strlen(cmd));
	         	argumentCount++;
	     	 }
	          cmd = strtok(NULL, " \n");
	      }
	    if (argumentCount == 0){
	    	return 1;
	    } else {
	    	myargv[argumentCount] = NULL;//important to clarify end of arguments
			return launchProgram(myargv);   	
		}
    }
	return 1;
}

void shellLoop(){
	int status;
	char **args;
 	do {
		output = 0;
 		write(STDOUT_FILENO, "whoosh> ", strlen("whoosh> "));
   	 	status = executeShell(args);
    } while (status != 0);
}

int main (int argc, char *argv[]){
	
	if (argc > 1 ){
		write(STDERR_FILENO, error_message, strlen(error_message));
		return 1;
	}
	shellLoop(); 
	return 0;
}

