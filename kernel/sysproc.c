#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  int num;
  uint64 va, retaddr;
  uint32 result = 0;
  argaddr(0, &va);
  argint(1, &num);
  argaddr(2, &retaddr);
  if (num > 32)
    return -1;
  pagetable_t pgtable = myproc()->pagetable;
  for (int i = 0; i < num; ++i)
  {
    if (va >= MAXVA)
      return -2;
    pte_t *pteaddr;
    if ((pteaddr = walk(pgtable, va, 0)) == 0)
      return -2;
    if (!(*pteaddr & PTE_V) || (*pteaddr & (PTE_R | PTE_W | PTE_X)) == 0)
      return -2;
    if (*pteaddr & PTE_A)
    {
      result |= 1 << i;
      *pteaddr ^= PTE_A;
    }
    va += PGSIZE;
  }
  copyout(pgtable, retaddr, (char *)&result, sizeof result);
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
