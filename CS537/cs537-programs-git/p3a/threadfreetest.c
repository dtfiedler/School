/*Test memory being freed in multiple threads. Assert that exactly one thread returns with a successful free.*/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"
#include <pthread.h>
const int THREADS = 2;

pthread_barrier_t barr;

int i;
int i2;
int count = -1;

void * ptr[10000];
int returnval[10000];
int returnval2[10000];

int ret = 1;

void * entry_point(void *arg) {
	int threadnum = *(int*) arg;
	
	i = 0;
	i2 = 0;
	
	int rc = pthread_barrier_wait(&barr);
    if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
	{
		printf("Could not wait on barrier\n");
		exit(-1);
	}
	if(threadnum == 0) {
		while(i < count) {
			returnval[i] = Mem_Free(ptr[i]);
			i++;
		}
	}
	else {
		while(i2 < count) {
			returnval2[i2] = Mem_Free(ptr[i2]);
			i2++;
		}
	}

	return (void *)&ret;
}

int main() {

	//Request memory
	
	assert(Mem_Init(4096*4) == 0);
	
    pthread_t thr[THREADS];

	//Barrier initialization
	if(pthread_barrier_init(&barr, NULL, THREADS))
	{
	    printf("Could not create a barrier\n");
		return -1;
	}
	
	int* p;
	int x[THREADS];
	
	int numtimes = 0;
	
	int j;

	for(numtimes = 0; numtimes < 10000; numtimes++) {
		
		ptr[0] = Mem_Alloc(16);
		count = 0;
		i=0;
		while(ptr[i] != NULL) {
			ptr[i+1] = Mem_Alloc(16);
			i++;
			count++;
		}

		for(j = 0; j < THREADS; ++j)
		{
			x[j] = j;
			p = &x[j];
			if(pthread_create(&thr[j], NULL, &entry_point, (void*)p))
			{
				printf("Could not create thread %d\n", j);
				return -1;
			}
		}
		//printf("Before join\n");
		for(j = 0; j < THREADS; ++j)
		{
			if(pthread_join(thr[j], NULL))
			{
				printf("Could not join thread %d\n", j);
				return -1;
			}
		}
		//printf("After join\n");
	
		i = 0;
		while(i < count) {
			printf("return value 1 is %d\n",returnval[i]);
			printf("return value 2 is %d\n",returnval2[i]);
			assert((returnval[i]>=0 && returnval2[i]<0) || (returnval[i]<0 && returnval2[i]>=0));
			ptr[i] = NULL;
			i++;
		}
		//printf("numtimes = %d\n", numtimes);
		//Mem_Dump();
	}


	printf("The test succeeded!\n");

	exit(0);
}
