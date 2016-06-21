#include <stdio.h>
#include "mfs.h"

int
main(){
	MFS_Init("localhost",3103);
	int rc = MFS_Creat(0,2,"rxy");
        printf("the rc num for the creat is %d\n",rc);
	rc = MFS_Lookup(0,"rxy");
	printf("the inode num for the abc is %d\n",rc);
	//printf("try to unlink abc");
	//rc = MFS_Unlink(0,"abc");
	//printf("the rc  for the unlink is %d\n",rc);
	//rc = MFS_Lookup(0,"abc");
        // printf("the inode num for the abc is %d\n",rc);
	
	MFS_Shutdown();

	return rc;
}
