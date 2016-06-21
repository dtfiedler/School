default: 
	gcc -c -Wall -Werror -fpic mem1.c -O
	gcc -shared -Wall -o libmem1.so mem1.o -O
	gcc -c -Wall -Werror -fpic mem2.c -O
	gcc -shared -Wall -o libmem2.so mem2.o -O
	gcc -c -Wall -Werror -fpic mem_contest_t.c -O
	gcc -shared -Wall -o libmem3.so mem_contest_t.o -O
contest:
	gcc -c -Wall -Werror -fpic mem_contest.c -O
	gcc -shared -Wall -o libmem_contest.so mem_contest.o -O
contest_x:
	gcc -c -Wall -Werror -fpic mem_contest_t.c -O
	gcc -shared -Wall -o libmem_contest_t.so mem_contest_t.o -O
test:
	gcc -lmem1 -L. -o test test.c -Wall -Werror
clean:
	rm -rf mem1.o libmem1.so mem2.o libmem2.so mem3.o libmem3.so
