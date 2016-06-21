#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}
int
sys_new_exec(void){
    char *path;
    char **args;
    int mx;
    if(argstr(0, &path) < 0 || argint(1, &mx) || argptr(2,((char**)(&args)),4) < 0){
    	cprintf("argint or argstr  went wrong inside new_exec\n");
	return -1;
    }
   // cprintf("path is %s, max_size is %d\n",path,max_stack_page);
   proc->max_stack_page = mx;
  // cprintf("the value of max_stack page after check is %d\n",max_stack_page);	
  // cprintf("the value of proc->max_stack page before assign is %d\n",proc->max_stack_page);
  // cprintf("the value of proc->max_stack page after assign is %d\n",proc->max_stack_page);
   return exec(path,args);
}
int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
 // cprintf("someone call sbrk\n");
 //	cprintf("stack_low_end is %d\n",proc->stack_low_bound);
   //     cprintf("addr is %d, n is %d\n",addr,n);
 //       cprintf("SOMETHING WORNG\n");
  if(proc->stack_low_bound !=0 && addr + n + PGSIZE > proc->stack_low_bound)
   {
	return -1;
   }
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
