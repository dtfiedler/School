#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "mem.h"

typedef struct block_hd{
	struct block_hd *next;
	struct block_hd *prev;
	int size;
} block_header;

typedef struct __myarg_t {
	block_header* start;
	int reverse;
	int size;	
} myarg_t;
typedef struct __myret_t {
	block_header* current;
	int status;
} myret_t;

block_header* list_head = NULL;
block_header* list_tail = NULL;
void *thread_space;
int search_done = 0;
pthread_mutex_t lock;
pthread_cond_t init;
int Mem_Init(int sizeOfRegion){
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&init, NULL);
	int fd;
	void* space_ptr;
	fd = open ("/dev/zero", O_RDWR);
	space_ptr = mmap(NULL,sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	thread_space = space_ptr;
	list_head = (block_header*)((char*)space_ptr + 2 * sizeof(myret_t));
	list_tail = (block_header*)((char*)space_ptr + sizeOfRegion - sizeof(block_header));
	list_head->next = list_tail;
	list_head->prev = NULL;
	list_head->size = sizeOfRegion - 2 * sizeof(myret_t)  - 2 * (int)sizeof(block_header);
	list_tail->next = NULL;
	list_tail->prev = list_head;
	list_tail->size = 0;
	close(fd);
	return 0;
}
void* List_Search(void* args){
	int reverse = ((myarg_t*)args)->reverse;
	int size = ((myarg_t*)args)->size;
	block_header* current = ((myarg_t*)args)->start;
	myret_t *ret;
	if(!reverse){ 
		ret = (myret_t*)thread_space;
	}
	else if(reverse){
		ret = ((myret_t*)thread_space) + 1;
	}
	if(!reverse){
		while(current){
			if(!(current->size & 0x0001)){
				if(size + sizeof(block_header) < current->size){
					ret->status = 1;
					ret->current = current;
					pthread_mutex_lock(&lock);
					search_done = 1;
					pthread_cond_signal(&init);
					pthread_mutex_unlock(&lock);
					return ret;
				}
				else if(size == current->size){
					ret->status = 0;
					pthread_mutex_lock(&lock);
					search_done = 1;
					pthread_cond_signal(&init);
					pthread_mutex_unlock(&lock);
					ret->current = current;
					return ret;
				}
			}
			current = current->next;
		}
	}
	else if(reverse){
		while(current){
			if(!(current->size & 0x0001)){
				if(size + sizeof(block_header) < current->size){
					ret->status = 1;
					ret->current = current;
					pthread_mutex_lock(&lock);
					search_done = 1;
					pthread_cond_signal(&init);
					pthread_mutex_unlock(&lock);
					return ret;
				}
				else if(size == current->size){
					ret->status = 0;
					ret->current = current;
					pthread_mutex_lock(&lock);
					search_done = 1;
					pthread_cond_signal(&init);
					pthread_mutex_unlock(&lock);
					return ret;
				}
			}
			current = current->prev;
		}
	}
	printf("WaaaH\n");
	return ret;
}	
void* Mem_Alloc(int size){
	pthread_t p1;
	pthread_t p2;
	myarg_t args1;
	myarg_t args2;
	args1.size = size; args1.start = list_head; args1.reverse = 0;
	args2.size = size; args2.start = list_tail; args2.reverse = 1;
	pthread_create(&p1, NULL, List_Search, &args1);
	pthread_create(&p2, NULL, List_Search, &args2);

	pthread_mutex_lock(&lock);
	while (search_done == 0)
		pthread_cond_wait(&init, &lock);
	pthread_mutex_unlock(&lock);
	//first fit algorithm
	if(size + sizeof(block_header) < current->size){
		//current block is bigger than require, split
		block_header *next = (block_header*)(((char*)(current+1)) + size);
		next->size = current->size -size - sizeof(block_header);
		next->next = current->next;
		next->prev = current;
		current->next = next;
		current->size = size;
		(current->size) ++;
		return (void*)(current+1);
	}
	else if(size == current->size){
		(current->size) ++;
		return (void*)(current+1);
	}				
	return NULL;
}

int Mem_Free(void *ptr){
	block_header* current = (block_header*)((char*)ptr -sizeof(block_header));
	(current->size)--;		
	if((current->next) && (!((current->next->size)&0x0001))){
		current->size += ((current->next->size) + sizeof(block_header));
		current->next = current->next->next;
		if(current->next)
			current->next->prev = current;
	}
	if((current->prev) && (!((current->prev->size)&0x0001))){
		current->prev->size += ((current->size) + sizeof(block_header));
		current->prev->next = current->next;
		if(current->next)
			current->next->prev = current->prev;
	}
	return 0;
}

