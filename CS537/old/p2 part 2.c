#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>


int main (int argc, char *argv[]){
	
	// process creation API

	// create another process (fork is a system call (a unix api to create a new process))
	//fork createas a new process, and also duplicates the current process

	int rc = fork();
	//new process created, new virtual address space!
	//prints goodbye twice! process is copy of parent
	//difference: return code
	if(rc == 0){ //child

		//creating a new process
		char *myargv[4];
		
		myargv[0] = strdup("/bin/ls");
		myargv[1] = strdup("-l");
		myargv[2] = strdup("-a");
		myargv[3] = NULL; //important, tells when arguments are done

		//before execv, want to change stdout
		close(STDOUT_FILENO);
		// //gives us bad file descriptor and does not work by iteslef. need to add below code

		open("/tmp/file.stdout", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
		//pushes our execv file output to our newley created file!
		//see it by entereing cat /tmp/file.stdout to terminal
		
		execv(myargv[0], myargv);
		printf("child: %d\n", (int) getpid());
	} else if (rc > 0){
		//parent
		int cpid = (int) wait(NULL);
		printf("parent: child pid was %d\n", cpid);
	} else {
		fprintf(stderr, "fail\n");
	}
	// want parent to wait for child to complete its execution using wait()
	// on successs wait returns the pid of terminated child

	// execv got rid of all the child stuff, now only called once
	printf("goodbye\n");

	return 0;
}


//for xv6 part, a lot of work will be in proc.


