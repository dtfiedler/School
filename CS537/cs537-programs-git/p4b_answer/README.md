me:Xuyi Ruan
CSL-ID:xuyi
Name:Yudong Sun
CSL-ID:yudong

(Leave the second name and ID blank if working alone)


##Project 4b: xv6 Threads  
 
Description:  
http://pages.cs.wisc.edu/~cs537-2/Projects/p4b.html  

Milestones:  

1. implement `int clone(void *stack)`, behave like fork  
 a) child and parent points to the same page directory entry  
 b) child and parent share the same address space  
 c) child will have its own stack (allocated from heap)  
 d) update child (esp, ebp) from the new stack  
 e) wrapper user-space function `int thread_create(void (*fn) (void *), void *arg)`  

2. `int lock(int *l)`  
-> if lock, call `sleep()` in `proc.c`  

3. `int unlock(int *l)`   
-> if unlock, `wakeup()` [in `proc.c`] the sleeping thread.  

4. `int join()`  
 
 
 

question:   
1. why copy whole page for stack?   
2. stack `esp, ebp` why not allign, why `ebp` is not the end of page?  
3. `thread_create` should return after setting up the essential information on new thread. but   
in the description said we have to free within `thread_create` which mean we have to wait within   
`thread_create`? what about `join()`?  
4. under what kind of circumstance, you want to grow the user space? 