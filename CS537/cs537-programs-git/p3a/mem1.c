#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include "mem.h"

int Math_Pow(int base, int exp);
int first_time_init = 1;
void* space_ptr;
int header_size;
int mem_available;
pthread_mutex_t mutex;
int Mem_Init(int sizeOfRegion){
	pthread_mutex_init(&mutex, NULL);
	//assert(rc==0);
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
	int seg_number = alloc_size / 16;
	header_size = seg_number/8;
	unsigned char* tmp = space_ptr;
	int i;
	for(i=0;i<header_size;i++){
		*(tmp+i) = 0x00;
	}
	int num_block_for_header = header_size / 16;	
	int num_char_for_header = num_block_for_header/8;
	for(i=0;i<num_char_for_header;i++){
		*(tmp+i) = 0xFF;
	}
	int num_char_offset_for_header = num_block_for_header%8;
	unsigned char *current_seg = tmp+i;
	for(i=0; i<num_char_offset_for_header; i++){
		//set i th bits to be 1;
		*current_seg = (*current_seg) | (0x80 >> i);
	}
	first_time_init = 0;
	close(fd);
	mem_available = alloc_size - header_size;
	//printf("the address of the space is %p\n",space_ptr);
	return 0;
}
	
void* Mem_Alloc(int size){
	//check size
	if(size <=0)
		return NULL;
	//first fit algorithm
	pthread_mutex_lock(&mutex);
	int i;
	for(i=0; i<header_size;i++){
		unsigned char *current_seg = (unsigned char*)space_ptr + i;
		//search inside each char
	//	printf("checking the %d th header\n",i);
		if((*current_seg) != 0xFF){
			int j;
			for(j=0; j<8; j++){
	//			printf("checking the %d th bit inside the %d th header\n",j,i);
				unsigned char current = (*current_seg) << j;
				if((current & 0x80) == 0){
					*current_seg = ((*current_seg) | (0x80 >> j));
			//		printf("going to return the %d th bit inside the %d th header\n",j,i);
				mem_available -= 16;
				pthread_mutex_unlock(&mutex);	
				return ((char*)space_ptr + (i*8+j)*16);
				}
			}
		}
	}
	pthread_mutex_unlock(&mutex);	
	return NULL;
}

int Mem_Free(void *ptr){
	pthread_mutex_lock(&mutex);
	if(ptr == NULL){
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	if(((((char*)ptr - (char*)space_ptr)) %  16) != 0){
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	int index = (((char*)ptr - (char*)space_ptr)) / 16;
	int sub_index_i = index / 8;
	int sub_index_j = index % 8;
	unsigned char *current_seg = (unsigned char*)space_ptr + sub_index_i;
	*current_seg = (*current_seg)  & ( Math_Pow(2,8) -1 - Math_Pow(2,8-sub_index_j+1));
	mem_available += 16;
	pthread_mutex_unlock(&mutex);
	return 0;
}

int Math_Pow(int base, int exp){
	if(exp < 0){
		printf("Not Supported\n");
		return -1;
	}
	int result = 1;
	int i;
	for(i=0;i<exp;i++)
		result *= exp;
	return result;
}

int Mem_Available(){
	return mem_available;
}
void Mem_Dump()
{
	printf("Not Supported\n");
	return;
}
