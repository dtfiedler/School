#include "types.h"
#include "stat.h"
#include "user.h"

int aa (int x, int y, int z)
{
	int b = x + y + z;
	return b;
}


int main (int argc, char* argv[])
{
	int a, b, c;
	a = 1;
	b = 2;
	c = 3;
	aa(a, b, c);
        exit();
}

