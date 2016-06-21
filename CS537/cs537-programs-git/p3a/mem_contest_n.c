#include <stdio.h>
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
	int magic_num;
} block_header;

block_header* list_head = NULL;
// nxt is the ptr for next fit algro.
block_header* nxt = NULL;
int first_time_init = 1;
int magic_num = 104114104;

int Mem_Init(int sizeOfRegion){
	if (first_time_init == 0) {
		fprintf(stderr, "Error! Init more than once.\n");
		return -1;
	}
	if(sizeOfRegion <= 0){
		fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
		return -1;
	}
	int fd = 0;
	int page_size = 0;
	void* space_ptr;
	page_size = getpagesize();
	if(sizeOfRegion % page_size != 0)
        sizeOfRegion = sizeOfRegion + (page_size-(sizeOfRegion % page_size));
	fd = open ("/dev/zero", O_RDWR);
	space_ptr = mmap(NULL,sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	list_head = (block_header*)space_ptr;
	list_head->next = NULL;
	list_head->prev = NULL;
	list_head->size = sizeOfRegion - (int)sizeof(block_header);
	list_head->magic_num = magic_num;
	nxt = list_head;
	first_time_init = 0;
	close(fd);
	return 0;
}

void* Mem_Alloc(int size){
	//next fit algorithm
    block_header* current = nxt;
	if(size % 8 != 0)
		size = size + (8-(size % 8));
	do{
        if(!(current->size & 0x0001)){
			//current block is free
			if(size + sizeof(block_header) < current->size){
				//current block is bigger than require, split
				block_header *next = (block_header*)(((char*)(current+1)) + size);
				next->size = current->size -size - sizeof(block_header);
				next->next = current->next;
				next->prev = current;
				next->magic_num = magic_num;
				current->next = next;
				current->size = size;
				(current->size) ++;
				// next search start from current->next
				nxt = next;
				// printf("%p\n", (void*)(current+1));
				return (void*)(current+1);
			}
			else if(size <= current->size){
				(current->size) ++;
				// next search start from current->next
				nxt = current->next;
				if(!nxt){
					//if alrady at the end of the list
					//reset next = list head
					nxt = list_head;
				}
				// printf("%p\n", (void*)(current+1));
				return (void*)(current+1);
			}
		}
		//otherwise skip it
		current = current->next;
		// @ end of the list, start from head.
		if (!current)
			current = list_head;
	} while (current != nxt);
	return NULL;
}

int Mem_Free(void *ptr){
	if (ptr == NULL) {
		printf("Error! tried to free NULL\n");
        return -1;
	}
	block_header* current = (block_header*)((char*)ptr -sizeof(block_header));
	// check bad free ptr
	if (current->magic_num != magic_num) {
		printf("Error! magic num check failed.\n");
        return -1;
	}
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
