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

	header_size = seg_number * 4 / 8;

	unsigned char* tmp = space_ptr;
	int i;
	for(i=0;i<header_size;i++){
		*(tmp+i) = 0x00;
	}
	int num_block_for_header = header_size / 16;
	int num_char_for_header = num_block_for_header * 4 / 8;
	for(i=0;i<num_char_for_header;i++){
		*(tmp+i) = 0x66;
		//0110 represent for 16 bytes in use
		//01100110 represents two 16 bytes are in use
	}
	int num_char_offset_for_header = num_block_for_header % 2;
	unsigned char *current_seg = tmp+i;
	if(num_char_offset_for_header == 1){
		*current_seg = 0x60;
	}
	first_time_init = 0;
	close(fd);
	//printf("num_block_for_header is %d\n",num_block_for_header);
	//printf("num_char_for_header is %d\n",num_char_for_header);
	//printf("header size is %d\n",header_size);
	//for(i=0; i< header_size; i++){
	//	printf("the content of %d th header is %d\n",i,(int)(*((unsigned char*)space_ptr + i)));	
	//}
	//printf("the address of the space is %p\n",space_ptr);
	mem_available = alloc_size - header_size;
	return 0;
}

void* Mem_Alloc(int size){
	if(size <=0)
		return NULL;
	if((size != 16) && (size != 80) && (size != 256))
		return NULL;
	int num_of_block_required = size / 16;
	//printf("num of block required is %d\n",num_of_block_required);
	unsigned char *addr = (unsigned char*)space_ptr;
	//printf("the address of space ptr is %p \n",addr);
	int i;
	int count = 0;
	int start_block_index_i = 0;
	int start_block_index_j = 0;
	pthread_mutex_lock(&mutex);
	for(i=0; i<header_size;i++){
		unsigned char *current_seg = (unsigned char*)space_ptr + i;
		int j;
		for(j=0; j<2; j++){
	//		printf("checking the %d part of %d char\n",j,i);
			unsigned char current = (*current_seg) << j*4;
			current = current & 0xF0;
			if(current == 0x00){
				if(count == 0){
					start_block_index_i = i;
					start_block_index_j = j;
				}
				count++;
				if(count == num_of_block_required){
					//mark the first blcok acoding to size, and then makr the rest of block to be 0010 to indicat
					// then compute the ptr should return;
					unsigned char mask;
					if(size == 16){
						mask = 0x06;
					}
					else if(size == 80){
						mask = 0x0A;
					}
					else if(size == 256){
						mask = 0x0E;
					}
					else{
						mask = -1;
						fprintf(stderr,"Maks Init to -1\n");
					}
	//				printf("value of mask is %d\n",mask);
	//				printf("start index of i %d\n",start_block_index_i);
	//				printf("start index of j %d\n",start_block_index_j);
					*(((unsigned char*)space_ptr) + start_block_index_i) = *(((unsigned char*)space_ptr) +start_block_index_i) | (mask << (1-start_block_index_j)*4);
					int m;
					for(m=1; m<num_of_block_required; m++){
						int l2_index = start_block_index_j + m;
						int l1_index = start_block_index_i;
						while(l2_index > 1){
							l2_index -= 2;
							l1_index++;
						}
						*(((unsigned char*)space_ptr) +l1_index) = (*(((unsigned char*)space_ptr) +l1_index)) | (0x02 << (1-l2_index) * 4);
					}
				//	for(i=0; i< 32; i++){
               			//	printf("the content of %d th header is %d\n",i,(int)(*((unsigned char*)space_ptr + i)));
        			//	}
				//	printf("the address going to return is %p\n",addr);
					mem_available -= size;
					pthread_mutex_unlock(&mutex);
					return addr;
				}
			}
			else{
				addr += count * 16;
				if((current == 0x60) ){
	//				printf("Skip 16 bytes\n");
					//skip 16 Bytes block
					addr += 16;
				}
				else if((current == 0xA0)){
					//skip 5 * 16 bytes blocks
	//				printf("Skip 5 * 16 bytes\n");
					addr += 5*16;
				}
				else if((current == 0xE0)){
					//skip 16 * 16 bytes blocks
	//				printf("Skip 16 * 16 bytes\n");
					addr += 16*16;
				}
				count = 0;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

int Mem_Free(void *ptr){
	if(ptr == NULL)
		return -1;
	if(((((char*)ptr - (char*)space_ptr)) %  16) != 0){
		return -1;
	}
	pthread_mutex_lock(&mutex);
	//printf("the ptr that is going to free is %p\n",ptr);
	int index = (((char*)ptr - (char*)space_ptr)) / 16;
	//printf("This is the %d th block\n",index);
	int sub_index_i = index / 2;
	int sub_index_j = index % 2;
	//printf("sub index i is %d\nsub index j is %d\n",sub_index_i,sub_index_j);
	unsigned char *current_seg = (unsigned char*)space_ptr + sub_index_i;
	unsigned char current = (*current_seg) << (sub_index_j)*4;
	current = current & 0xF0;
	int size;
	//printf("current free correspoding to a  %d block\n",current);
        if(current == 0x60){
		size = 16;
	}
	else if(current == 0xA0){
		size = 5*16;
	}
	else if(current == 0xE0){
		size = 16*16;
	}
	else{
		size = -1;
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	int m;
	for(m=0; m < size / 16; m++){
		int l2_index = sub_index_j + m;
                int l1_index = sub_index_i;
		while(l2_index > 1){
                        l2_index -= 2;
                        l1_index++;
                }
	*(((unsigned char*)space_ptr) +l1_index) = *(((unsigned char*)space_ptr) +l1_index) & (0x0F << l2_index * 4);	
	}
	mem_available += size;
	pthread_mutex_unlock(&mutex);
	//int i;
	//for(i=0; i< 32; i++){
        //	printf("the content of %d th header is %d\n",i,(int)(*((unsigned char*)space_ptr + i)));
        //}
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
