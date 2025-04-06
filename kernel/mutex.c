#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "proc.h"
#include "mutex.h"

int is_log = 1;

struct file*
mutexalloc(void)
{
  struct file *f = filealloc();
  if(!f) {
    if(is_log){
      printf("mutexalloc: filealloc failed for process %d\n", myproc()->pid);
    }
    return 0;
  }

  struct mutex *m = (struct mutex*)kalloc();
  if(!m){
    if(is_log){
      printf("mutexalloc: kalloc failed for process %d\n", myproc()->pid);
    }
    fileclose(f);
    return 0;
  }

  memset(m, 0, sizeof(*m));
  initsleeplock(&m->lock, "mutex");
  initlock(&m->owner_lock, "mutex_owner");
  m->owner = -1;

  f->type = FD_MUTEX;
  f->readable = 0;
  f->writable = 0;
  f->mtx = m;

  if(is_log){
    printf("mutexalloc: process %d created mutex at %p\n", myproc()->pid, m);
  }

  return f;
}

void
mutexclose(struct file *f)
{
  if(is_log){
    printf("mutexclose: process %d closing mutex %p\n", myproc()->pid, f->mtx);
  }
  acquire(&f->mtx->owner_lock);
  if(f->mtx->owner == myproc()->pid){
    if(is_log){
      printf("mutexclose: process %d releasing lock on mutex %p\n", myproc()->pid, f->mtx);
    }
    releasesleep(&f->mtx->lock);
    f->mtx->owner = -1;
  }
  release(&f->mtx->owner_lock);

  if(is_log){
    printf("mutexclose: process %d freeing mutex %p\n", myproc()->pid, f->mtx);
  }

  kfree((char*)f->mtx);
  f->mtx = 0;
}
