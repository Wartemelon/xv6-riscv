#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
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
  if(n < 0)
    n = 0;
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

uint64 
sys_add(void) {
	int arg1, arg2;
	argint(0, &arg1);
	argint(1, &arg2);

	return arg1 + arg2;
}


#define PGINFO_FL_D 0x1
#define PGINFO_FL_A 0x2

void 
get_pgtableinfo(pagetable_t pagetable, int level, uint64 start, uint64 end, int flags, int depth) {
  for (int i = 0; i < NPTENTRIES; i++) {
      uint64 va = ((uint64)i) << (PGSHIFT + level * 9);
      if (va + PGSIZE <= start || va >= end) {
        continue;
      }

      pte_t pte = pagetable[i];
      if (!(pte & PTE_V)) {
        continue;
      }
      if ((pte & (PTE_R|PTE_W|PTE_X)) == 0) {
        for (int j = 0; j < depth * 9; j++)
          printf(".");

        printf("0x%d -> %p (table)\n", i, (void*)PTE2PA(pte));
        get_pgtableinfo((pagetable_t)PTE2PA(pte), level - 1, start, end, flags, depth + 1);
        continue;
      }
      if (flags) {
        int match = 0;
        if ((flags & PGINFO_FL_D) && (pte & PTE_D)) {
          match = 1;
        }
        if ((flags & PGINFO_FL_A) && (pte & PTE_A)) {
          match = 1;
        }
        if (!match) {
          continue;
        }
      }

      for (int j = 0; j < depth * 9; j++) {
        printf(".");
      }
      printf("0x%d -> %p ", i, (void*)PTE2PA(pte));
      printf("%s", (pte & PTE_R) ? "R" : "_");
      printf("%s", (pte & PTE_W) ? "W" : "_");
      printf("%s", (pte & PTE_X) ? "X" : "_");
      printf("%s", (pte & PTE_U) ? "U" : "_");
      printf("%s", (pte & PTE_G) ? "G" : "_");
      printf("%s", (pte & PTE_A) ? "A" : "_");
      printf("%s\n", (pte & PTE_D) ? "D" : "_");
  } 
}

uint64
sys_pgtableinfo(void) {
  uint64 buf, buflen;
  int flags;
  argaddr(0, &buf);
  argaddr(1, &buflen);
  argint(2, &flags);

  if (flags & ~(PGINFO_FL_D | PGINFO_FL_A)) {
    return -1;
  }

  uint64 start, end;
  if (buf == 0 || buflen == 0) {
    start = 0;
    end = MAXVA;
  }
  else {
    start = PGROUNDDOWN(buf);
    end = PGROUNDUP(buf + buflen);
  }

  if (buf != 0 || buflen != 0) {
    pte_t *pte = walk(myproc()->pagetable, PGROUNDDOWN(start), 0);
    if (!pte || !(*pte & PTE_V)) {
        return -1;
    }
    printf("PAGETABLE %p\n", (void*)PTE2PA(*pte));
  } 
  else {
    printf("PAGETABLE %p\n", (void*)PTE2PA(myproc()->pagetable[0]));
  }
  get_pgtableinfo(myproc()->pagetable, 2, start, end, flags, 0);
  return 0;
}

#define PGFLAGCLEAR_D 0x1
#define PGFLAGCLEAR_A 0x2

int 
clear_pgflags(pagetable_t pagetable, int level, uint64 start, uint64 end, int flags) {
  for (int i = 0; i < NPTENTRIES; i++) {
    uint64 va = ((uint64)i) << (PGSHIFT + level * 9);
    if (va + PGSIZE <= start || va >= end) {
      continue;
    }

    pte_t *pte = &pagetable[i];
    if (!(*pte & PTE_V)) {
      continue;
    }
    if ((*pte & (PTE_R|PTE_W|PTE_X)) == 0) {
      int r = clear_pgflags((pagetable_t)PTE2PA(*pte), level - 1, start, end, flags);
      if (r != 0) {
        return r;
      }
      continue;
    }

    if (flags & PGFLAGCLEAR_D) {
      *pte &= ~PTE_D;
    }
    if (flags & PGFLAGCLEAR_A) {
      *pte &= ~PTE_A;
    }
  }
  sfence_vma();
  return 0;
}

uint64
sys_clr_pgflags(void) {
  uint64 buf, buflen;
  int flags;
  argaddr(0, &buf);
  argaddr(1, &buflen);
  argint(2, &flags);
  
  if (flags & ~(PGFLAGCLEAR_D | PGFLAGCLEAR_A)) {
    return -1;
  }
  uint64 start, end;
  if (buf == 0 || buflen == 0) {
    start = 0;
    end = MAXVA;
  }
  else {
    start = PGROUNDDOWN(buf);
    end = PGROUNDUP(buf + buflen);
  }
  return clear_pgflags(myproc()->pagetable, 2, start, end, flags);
}