default: 
	gcc -c -Wall -Werror -fpic mfs.c -O
	gcc -shared -Wall -o libmfs.so mfs.o -O
	gcc -Wall -Werror -o server server.c -O
test:
	gcc -lmfs -L. -o test test.c -Wall -Werror
clean:
	rm -rf mfs.o udp.o libudp.so libmfs.so server test
