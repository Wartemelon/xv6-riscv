#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "proc.h"
#include "mutex.h"

struct file*
mutexalloc(void)
{
  struct file *f = filealloc();
  if(!f)
    return 0;

  struct mutex *m = (struct mutex*)kalloc();
  if(!m){
    fileclose(f);
    return 0;
  }

  memset(m, 0, sizeof(*m));
  initsleeplock(&m->lock, "mutex");
  m->owner = -1;

  f->type = FD_MUTEX;
  f->readable = 0;
  f->writable = 0;
  f->mtx = m;

  return f;
}

void
mutexclose(struct file *f)
{
  if(f->mtx->owner == myproc()->pid){
    releasesleep(&f->mtx->lock);
    f->mtx->owner = -1;
  }

  kfree((char*)f->mtx);
  f->mtx = 0;
}
