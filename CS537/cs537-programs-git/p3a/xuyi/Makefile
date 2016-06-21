default: 
	gcc -c -Wall -Werror -fpic mem1.c -O
	gcc -shared -Wall -o libmem1.so mem1.o -O
	gcc -c -Wall -Werror -fpic mem2.c -O
	gcc -shared -Wall -o libmem2.so mem2.o -O
	gcc -c -Wall -Werror -fpic mem3.c -O
	gcc -shared -Wall -o libmem3.so mem3.o -O
test: test.c
	gcc -lmem -L. -o test test.c -Wall -Werror
clean:
	rm -rf mem.o libmem.so
