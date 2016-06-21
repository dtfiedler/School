#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"


int
main(int argc, char *argv[])
{
	printf(1, "counter = %d\n", addnum(3));
	printf(1, "counter = %d\n", addnum(2));
	printf(1, "counter = %d\n", addnum(9));
	exit();
}
