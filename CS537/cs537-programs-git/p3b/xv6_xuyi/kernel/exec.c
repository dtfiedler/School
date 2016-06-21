#include "types.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;

  if((ip = namei(path)) == 0)
    return -1;
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // the code BELOW deals with the CODE segement of the adress space.
  // 
  // Load program into memory.
  sz = PGSIZE;
  //cprintf("@CODE start:%d\n", sz);
  
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    //cprintf("Allocate memory in the user space\n");
    //cprintf("start= %d, va=%d, memsz=%d, size=%d\n", sz, ph.va, ph.memsz, ph.va + ph.memsz);
    // why we add PGSIZE here? does ph.va change?
    if((sz = allocuvm(pgdir, sz, ph.va + ph.memsz + PGSIZE)) == 0) {
	goto bad;
    }
    if(loaduvm(pgdir, (char*)ph.va, ip, ph.offset, ph.filesz) < 0)
	goto bad;
  }
  iunlockput(ip);
  ip = 0;
  //cprintf("@CODE end:%d\n", sz);

  // The code ABOVE deal with CODE segement

  // Program stack @start 
  // Allocate a one-page stack at the next page boundary
  // this is the stack allocation part.
  // TODO
  // unlock the stack ability to become not limited to only ONE page.

  sz = PGROUNDUP(sz);
  // since the queue will initialize with the value sz 
  // implicitly after the allocation of stack is done
  // we do not want to modify the value of sz 
  // we replace sz with sp below. 

  /////////////////////
  //     SEC 3.1   ///
  ///////////////////
  sp = USERTOP;
  proc->stack_tp=sp-PGSIZE;
  //cprintf("@STACK start:%d\n", sp);
  // cprintf("start= %d, size=%d\n", sp, sp+PGSIZE);
  if((sp = allocuvm(pgdir, sp-PGSIZE,  sp)) == 0)
    goto bad;
  
  //cprintf("@STACK end:%d\n", sp);
  
  // code below not need to modify 
  // Push argument strings, prepare rest of stack in ustack.
  
  // change sp to USERTOP to relocate the stack to the buttom of addr space.
  // sp = sz;
  // sz is the end of the stack ptr
  // in the original arrangment of the address space for program
  // the stack also grows backwards. therefore, not much changes are 
  // need to do below..
  
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp &= ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  //cprintf("@STACK sp now!:0x%d\n", sp);
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  // minus - stack grows backward
  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc->name, last, sizeof(proc->name));

  // Commit to the user image.
  oldpgdir = proc->pgdir;
  proc->pgdir = pgdir;
  proc->sz = sz;
  proc->tf->eip = elf.entry;  // main
  proc->tf->esp = sp;
  switchuvm(proc);
  freevm(oldpgdir);
  // cprintf("proc->pid:%d\n",proc->pid);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip)
    iunlockput(ip);
  return -1;
}
