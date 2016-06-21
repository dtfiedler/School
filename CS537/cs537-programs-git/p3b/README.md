---
Author: Xuyi Ruan  
Date:   10/27/2014  
Course: CS537  

---
##p3b partA:
Null-pointer Derefernece

TODO:  
1. check 'fork() and exec()' function    
2. change how programs are loaded so that it will not start @0x00  
3. make sure segfault(display err) when some tries to access @0x00  

Steps:  
1. in kernel/exec.c, i chanted sz = 0 to sz= PGSIZE  
2. in kernel/vm.c, i changed i = 0 to i = PGSIZE  
3. in user/makefile.mk, changed ox0 to 0x1000  

Dont forget to 'make clean' after you make changes!

##p3b partB:
Stack Rearrangement

-> current address space arrangment for xv6  
//////  
code  
//////  
stack(fixed-size, one page)  
/////  
heap(grows toward the end of address space)

-> modified arrangement of new address space  
/////  
code  
//////  
heap (grows toward the end of address space)  
//////  
stack (at end of address space, grows backward, grows up!)  

TODO:  
stack (grows backward)  
1. move location of stack to the end of the address space (sp = USERTOP)  
2. add expand ability
3. allocate a page in btween stack and heap to prevent them from hitting 
each other. 
ex. when heap in page1, stack will grows from the end of addr space up to page3 of the addr space, so that it wont crash with heap.

NOTES:
 
1. Move ONE-page size stack into the buttom of the stack  
-> change stack ptr 'sp' sec 3.1, in exec.c   
-> add boundrary @fetchint @fetchstr @argptr in syscall.c  
Explanation:  
-> new stack is now reallocated to the buttom of addr space  
-> the previous boundrary checking need to address for the new location of stack.  

2. unlock the limition of ONEPAGE stack.  
-> stack already grows upward, good news for us!  
-> what do we need to change to grow stack?  
-> stack expand upward, when process needs more space in stack  
-> if the requre addr is within one page of the stack, allocated it.  
-> how doe we allocated?   
-> add SEGFAULT handling in trap.c  


3. protest heap from growing into stack  
-> add boundary checking in sys_sbrk() in sysproc.c  
-> sys_sbrk() checks boundrary before calling growproc() here;  










