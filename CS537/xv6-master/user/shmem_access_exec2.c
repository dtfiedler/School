#include "types.h"
#include "stat.h"
#include "user.h"

void
test_failed()
{
	printf(1, "TEST FAILED\n");
	exit();
}

void
test_passed()
{
 printf(1, "TEST PASSED\n");
 exit();
}

char *args[] = { "echo", 0 };

int
main(int argc, char *argv[])
{
	void *ptr;
	int i;

	for (i = 0; i < 4; i++) {
		ptr = shmem_access(i);
		if (ptr == NULL) {
			test_failed();
		}
	}
	
	int pid = fork();
	if (pid < 0) {
		test_failed();
	}
	else if (pid == 0) {
    exec("echo", args);
    printf(1, "exec failed!\n");
    test_failed();
		exit();	
	}
	else {
		wait();
	}
  
	exit();
}
