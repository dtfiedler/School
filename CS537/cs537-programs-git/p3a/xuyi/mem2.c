/******************************************************************************
 * FILENAME: mem.c
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * DATE:     20 Nov 2013
 * PROVIDES: Contains a set of library functions for memory allocation
 * MODIFIED BY: Xuyi Ruan, section#1
 * DATE:     05 May 2014
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/* this structure serves as the header for each block */
typedef struct block_hd{
    /* The blocks are maintained as a linked list */
    /* The blocks are ordered in the increasing order of addresses */
    struct block_hd* next;
    
    /* size of the block is always a multiple of 4 */
    /* LSB = 0 => free block */
    /* LSB = 1 => allocated/busy block */
    
    /* For free block, block size = size_status */
    /* For an allocated block, block size = size_status - 1 */
    
    /* The size of the block stored here is not the real size of the block */
    /* the size stored here = (size of block) - (size of header) */
    int size_status;
    
}block_header;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
block_header* list_head = NULL;


/* Function used to Initialize the memory allocator */
/* Not intended to be called more than once by a program */
/* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated */
/* Returns 0 on success and -1 on failure */
int Mem_Init(int sizeOfRegion)
{
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    static int allocated_once = 0;
    
    if(0 != allocated_once)
    {
        fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
        return -1;
    }
    if(sizeOfRegion <= 0)
    {
        fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
        return -1;
    }
    
    /* Get the pagesize */
    pagesize = getpagesize();
    
    /* Calculate padsize as the padding required to round up sizeOfRegio to a multiple of pagesize */
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;
    
    alloc_size = sizeOfRegion + padsize;
    
    /* Using mmap to allocate memory */
    fd = open("/dev/zero", O_RDWR);
    if(-1 == fd)
    {
        fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == space_ptr)
    {
        fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
    
    allocated_once = 1;
    
    /* To begin with, there is only one big, free block */
    list_head = (block_header*)space_ptr;
    list_head->next = NULL;
    /* Remember that the 'size' stored in block size excludes the space for the header */
    list_head->size_status = alloc_size - (int)sizeof(block_header);
    
    return 0;
}


/* Function for allocating 'size' bytes. */
/* Returns address of allocated block on success */
/* Returns NULL on failure */
/* Here is what this function should accomplish */
/* - Check for sanity of size - Return NULL when appropriate */
/* - Round up size to a multiple of 4 */
/* - Traverse the list of blocks and allocate the first free block which can accommodate the requested size */
/* -- Also, when allocating a block - split it into two blocks when possible */
/* Tips: Be careful with pointer arithmetic */
void* Mem_Alloc(int size)
{
    
    /* - Check for sanity of size - Return NULL when appropriate */
    int requestSize = size;
    if (requestSize <= 0)
        return NULL;
    /* round size to a multiple of 4 is size mod 4 is not zero */
    if (requestSize % 4 != 0)
    {
        requestSize = ((requestSize/4)+1)*4;
    }
    
    // let curPtr point to the start of first block
    block_header* curPtr = list_head;
    // when user requests size = 24 for example, free block must has at least
    // 24+8 = 32 to allocate.
    while(((curPtr->size_status & 1) == 1) | (curPtr->size_status < requestSize+8))
    {
        curPtr = curPtr->next;
        if(curPtr == NULL)
        {
            return NULL;
        }
    }
    
    void* rtnPtr = (char*)curPtr + (int)sizeof(*curPtr);
    
    // nextPtr - the pointer for next free block
    block_header* nextPtr = NULL;
    nextPtr =(block_header*) ((char*)curPtr + (int)sizeof(*curPtr) + requestSize);
    
    nextPtr->size_status = curPtr->size_status - requestSize - (int)sizeof(*nextPtr);
    nextPtr->next = curPtr->next;
    
    // the size of the allocated block became size + 1 <- 1 indicated allocated
    curPtr->size_status = requestSize + 1;
    curPtr->next = nextPtr;
    
    return rtnPtr;
}

/* Function for freeing up a previously allocated block */
/* Argument - ptr: Address of the block to be freed up */
/* Returns 0 on success */
/* Returns -1 on failure */
/* Here is what this function should accomplish */
/* - Return -1 if ptr is NULL */
/* - Return -1 if ptr is not pointing to the first byte of a busy block */
/* - Mark the block as free */
/* - Coalesce if one or both of the immediate neighbours are free */
int Mem_Free(void *ptr)
{
    /* - Return -1 if ptr is NULL */
    if (ptr == NULL)
        return -1;
    
    // if the ptr is not allocated by Mem_Alloc
    if ((unsigned long int)ptr % 4 != 0)
        return -1;
    
    /* - Return -1 if ptr is not pointing to the first byte of a busy block */
    block_header* curPtr = (block_header*) ((char*)ptr - (int)sizeof(*list_head));
    
    // added 2 @ may 6th 21:19
    // make sure that curPtr is the pointer to a header.
    // if not, ptr is not a valid pointer to a space to be freed.
    if (curPtr->size_status % 4 != 1)
        return -1;
    
    // added 1 @ may 6th morning
    // when there the ptr is not allocatd by Mem_Alloc
    if(sizeof(*curPtr) != sizeof(block_header))
        return -1;
    
    // if the curPtr is not the pointer to a allocated (LSB = 1) return -1
    if((curPtr->size_status & 1) != 1)
        return -1;
    
    // create a traversing pointer called travPtr
    block_header* travPtr = NULL;
    travPtr = list_head;
    while (travPtr->next != curPtr && travPtr != curPtr)
    {
        travPtr = travPtr->next;
    }
    
    // prePtr points to the block above 'curPtr' block
    block_header* prePtr = travPtr;
    // nextPtr points to the block below 'curPtr'
    block_header* nextPtr = curPtr->next;
    // when there are free space above 'curPtr' to be coalesced. header is on the
    // previous block.
    if ((prePtr->size_status & 1) == 0)
    {
        // coalesce the previous free and current block
        prePtr->size_status = prePtr->size_status + (int)sizeof(*curPtr) + curPtr->size_status - 1;
        prePtr->next = curPtr->next;
        curPtr = prePtr;
        // if the next block of current block is also free
        // the curPtr points to the partially already coalesced blocks.
        // (above + current), therefore the 'curPtr->size_status' now
        // not need to -1.
        if ((nextPtr->size_status & 1) == 0)
        {
            curPtr->size_status = curPtr->size_status + (int)sizeof(*nextPtr) + nextPtr->size_status;
            curPtr->next = nextPtr->next;
        }
        return 0;
    }
    
    // when the block above 'curPtr' is busy, header on 'curPtr' block.
    // when the block below is not busy, coalese it with current block.
    if ((nextPtr->size_status & 1) == 0)
    {
        curPtr->size_status = curPtr->size_status - 1 + (int)sizeof(*nextPtr) + nextPtr->size_status;
        curPtr->next = nextPtr->next;
        return 0;
    }
    
    // otherwise, the above and below neighbors are all busy,
    // just free the current block
    curPtr->size_status = curPtr->size_status - 1;
    
    return 0;
}

/* Function to be used for debug */
/* Prints out a list of all the blocks along with the following information for each block */
/* No.      : Serial number of the block */
/* Status   : free/busy */
/* Begin    : Address of the first useful byte in the block */
/* End      : Address of the last byte in the block */
/* Size     : Size of the block (excluding the header) */
/* t_Size   : Size of the block (including the header) */
/* t_Begin  : Address of the first byte in the block (this is where the header starts) */
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
    fprintf(stdout,"************************************Block list***********************************\n");
    fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tSize\tt_Size\tt_Begin\n");
    fprintf(stdout,"---------------------------------------------------------------------------------\n");
    while(NULL != current)
    {
        t_Begin = (char*)current;
        Begin = t_Begin + (int)sizeof(block_header);
        Size = current->size_status;
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
        fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
        total_size = total_size + t_Size;
        current = current->next;
        counter = counter + 1;
    }
    fprintf(stdout,"---------------------------------------------------------------------------------\n");
    fprintf(stdout,"*********************************************************************************\n");
    
    fprintf(stdout,"Total busy size = %d\n",busy_size);
    fprintf(stdout,"Total free size = %d\n",free_size);
    fprintf(stdout,"Total size = %d\n",busy_size+free_size);
    fprintf(stdout,"*********************************************************************************\n");
    fflush(stdout);
    return;
}
