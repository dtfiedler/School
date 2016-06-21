#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

#define RESERVE 1;
#define SPOT 0;

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

// TODO
// reserve sys_call
int 
sys_reserve (void)
{
 int percent;

 if(argint(0, &percent) < 0)
    return -1;
 if(percent < 0 || percent > 100)
    return -1;
 total += percent;
 if (total > 200)
    return -1;
 proc->sche_type = RESERVE;
 proc->sche_para = percent;
 return 0;
}


// TODO
// spot sys_call
int
sys_spot (void)
{
 int bid;
  if(argint(0, &bid) < 0)
    return -1;
  if(bid < 0)
    return -1;
  proc->sche_type = SPOT;
  proc->sche_para = bid;
  return 0;
}


// getpinfo sys_call
int
sys_getpinfo(void)
{
 //argptr(proc);
 //fill_pstat(proc);
 cprintf("getpinfo");
 return 0;
}


int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
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
