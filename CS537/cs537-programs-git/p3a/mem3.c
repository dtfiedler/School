#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"


typedef struct block_hd{
	struct block_hd *next;
	int size;
	int magic_num;
} block_header;
pthread_mutex_t mutex;
int mem_available;
block_header* list_head = NULL;
int first_time_init = 1;
int magic_num = 103103103;
int Mem_Init(int sizeOfRegion){
	pthread_mutex_init(&mutex, NULL);
	if(first_time_init == 0){
		fprintf(stderr,"Error:mem.c: Init mroe than once\n");
                return -1;
	}
	if(sizeOfRegion <= 0){
		fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
		return -1;
	}
	int page_size;
	int pad_size;
	int fd;
	int alloc_size;
	void* space_ptr;
	page_size = getpagesize();
	pad_size = sizeOfRegion %  page_size;
	if(pad_size !=0)
		pad_size = page_size - pad_size;
	alloc_size = sizeOfRegion + pad_size;
	fd = open ("/dev/zero", O_RDWR);
	if(fd == -1){
		fprintf(stderr,"Cannot opne /dev/zero\n");
		return -1;
	}
	space_ptr = mmap(NULL,alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(space_ptr == MAP_FAILED){
		fprintf(stderr,"Map failed\n");
                return -1;
	}
	list_head = (block_header*)space_ptr;
	list_head->next = NULL;
	list_head->size = alloc_size - (int)sizeof(block_header);
	list_head->magic_num = magic_num;
	first_time_init = 0;
	close(fd);
	return 0;
}
	
void* Mem_Alloc(int size){
	//check size
	if(size <=0)
		return NULL;
	//round up size to a multiple of 8
	if(size%8!=0)
		size = size + (8-(size %8));
	//first fit algorithm
	block_header* current = list_head;
	pthread_mutex_lock(&mutex);
	while(current != NULL){
		if((current->size % 8) == 0){
			//current block is free
			if(size + sizeof(block_header) < current->size){
				//current block is bigger than require, split
				block_header *next = (block_header*)(((char*)(current+1)) + size);
				next->size = current->size -size - sizeof(block_header);
				next->next = current->next;
				next->magic_num = magic_num;
				current->next = next;
				current->size = size;
				(current->size) ++;
				pthread_mutex_unlock(&mutex);
				return (void*)(current+1);
			}
			else if(size  <= current->size){
				(current->size) ++;
				pthread_mutex_unlock(&mutex);
				return (void*)(current+1);
			}
		}
		//otherwise skip it
		current = current->next;
	}
	pthread_mutex_unlock(&mutex);	
	return NULL;
}

int Mem_Free(void *ptr){
	if(ptr == NULL)
		return -1;
	pthread_mutex_lock(&mutex);
	block_header* current = list_head;
	int valid = 0;
	while(current != NULL){
		if((char*)ptr - sizeof(block_header) == (char*)current){
			valid = 1;
			break;
		}
		current=current->next;
	}
	if(valid != 1){
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	if(((block_header*)((char*)ptr - sizeof(block_header)))->magic_num != magic_num){
		printf("Magic Num Check Fail\n");
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	if(((block_header*)((char*)ptr - sizeof(block_header)))->size % 8 != 1){
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	(((block_header*)((char*)ptr - sizeof(block_header)))->size)--;	
	//coalesce
	if((((block_header*)((char*)ptr - sizeof(block_header)))->next->size)%8 == 0){
		((block_header*)((char*)ptr - sizeof(block_header)))->size += ((((block_header*)((char*)ptr - sizeof(block_header)))->next->size) + sizeof(block_header));
		((block_header*)((char*)ptr - sizeof(block_header)))->next = ((block_header*)((char*)ptr - sizeof(block_header)))->next->next;
	}
	current = list_head;
	while(current->next!=NULL){
		if(current->next == ((block_header*)((char*)ptr - sizeof(block_header)))){
			if((current->size)%8==0){
				current->size += (current->next->size + sizeof(block_header));
				current->next = current->next->next;
				break;
			}
		}
		current = current->next;		
	}
	pthread_mutex_unlock(&mutex);	
	return 0;
}

void Mem_Dump()
{
  int counter;
  block_header* current = NULL;
  char* t_Begin = NULL;
  char* Begin = NULL;
  int Size;
  int t_Size;
  char* End = NULL;
  int free_size;
  int busy_size;
  int total_size;
  char status[5];

  free_size = 0;
  busy_size = 0;
  total_size = 0;
  current = list_head;
  counter = 1;
//  fprintf(stdout,"************************************Block list***********************************\n");
//  fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tSize\tt_Size\tt_Begin\n");
//  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  while(NULL != current)
  {
    t_Begin = (char*)current;
    Begin = t_Begin + (int)sizeof(block_header);
    Size = current->size;
    strcpy(status,"Free");
    if(Size & 1) /*LSB = 1 => busy block*/
    {
      strcpy(status,"Busy");
      Size = Size - 1; /*Minus one for ignoring status in busy block*/
      t_Size = Size + (int)sizeof(block_header);
      busy_size = busy_size + t_Size;
    }
    else
    {
      t_Size = Size + (int)sizeof(block_header);
      free_size = free_size + t_Size;
    }
    End = Begin + Size;
//    fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
    total_size = total_size + t_Size;
    current = current->next;
    counter = counter + 1;
  }
//  fprintf(stdout,"---------------------------------------------------------------------------------\n");
//  fprintf(stdout,"*********************************************************************************\n");
	mem_available = free_size;
}
int Mem_Available(){
	Mem_Dump();
	return mem_available;
}
