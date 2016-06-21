 #include <stdio.h>
#include <stdlib.h>

/* linear page table:
	PTE for VPN 0		[valid = 0 | protection | present| Physical Frame Number] //THIS PAGE IS NOT VALID EVER!
	PTE for VPN 1      	[valid = 1| protection | present| Physical Frame Number]
	PTE for VPN 2
	PTE for VPN 3
	PTE for VPN 4
	...

		note: pages between stack and heap are also invalid, will cause fault
*/

 int main (int argc, char* argv[]){

 	/***************INTRO********************/
 	int *p = (int*)malloc(sizeof(int)); //NULL pointer to an integer (often written as int*   p)
 	printf("code %p\n", main);
 	printf("stack %p\n", p);
 	printf("heap %p\n", &p);
 	p = 0;
 	//printf("%p\n", p); //prints out pointer 
 	
 	//*p = 100; //go to the address of p and set it to 100 : RESULTS IN SEGMENTATION FAULT! TRYING TO ACCESS VIRTRUAL ADDRESS OF 0
 	/****************************************/
 	return 0;
 }


//copy code
//tar xvzf /p/course/cs537-remzi/xv6

//create new user program

/*NOTES

	xv6 address space:
		code <- starts at VPN 0
		code
		stack <-1 page stack
		heap
			| 		(grows downward)
			v




 #include "types.h"
 #include "user.h"

 int main (int argc, char*argv[]){
 	int *p = 0; //null pointer
 	printf(1, "hello[%x]\n", *p); //wil it work?
 	exit();
 }
 */

























 