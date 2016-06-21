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
} block_header;

block_header* list_head = NULL;

int Mem_Init(int sizeOfRegion){
	int fd;
	void* space_ptr;
	fd = open ("/dev/zero", O_RDWR);
	space_ptr = mmap(NULL,sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	list_head = (block_header*)space_ptr;
	list_head->next = NULL;
	list_head->prev = NULL;
	list_head->size = sizeOfRegion - (int)sizeof(block_header);
	close(fd);
	return 0;
}
	
void* Mem_Alloc(int size){
	//first fit algorithm
	block_header* current = list_head;
	while(current != NULL){
		if(!(current->size & 0x0001)){
			//current block is free
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
			
		}
		//otherwise skip it
		current = current->next;
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

