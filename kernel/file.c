//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

#define PSEUDO_MAJOR 2

static uint64 urandom_seed = 88005553535ULL;
static struct spinlock urandom_lock;
static uint64 nullstat_count = 0;
static struct spinlock nullstat_lock;
static char buf[PGSIZE];

static unsigned char
lcg_byte(void) {
  urandom_seed = urandom_seed * 1103515245 + 12345;
  return (urandom_seed >> 16) & 0xFF;
}

static int
pseudo_read(int dev, uint64 dst, int n) {
  if(n > PGSIZE) return -1;
  int mi = minor(dev);

  switch(mi) {
  case 0:  // /dev/null
    return 0;
  case 1:  // /dev/zero
    memset(buf, 0, n);
    either_copyout(1, dst, buf, n);
    return n;
  case 2:  // /dev/urandom
    acquire(&urandom_lock);
    for(int i = 0; i < n; i++) buf[i] = lcg_byte();
    release(&urandom_lock);
    either_copyout(1, dst, buf, n);
    return n;
  case 3:  // /dev/nullstat
    if(n != sizeof(nullstat_count))
      return -1;
    acquire(&nullstat_lock);
    memmove(buf, &nullstat_count, n);
    release(&nullstat_lock);
    either_copyout(1, dst, buf, n);
    return n;
  default:
    return -1;
  }
}

static int
pseudo_write(int dev, uint64 src, int n) {
  if(n > PGSIZE) return -1;
  int mi = minor(dev);

  switch(mi) {
  case 0:  // /dev/null
    return n;
  case 1:  // /dev/zero — запись запрещена
    return -1;
  case 2:  // /dev/urandom — смена seed
    if(n != sizeof(urandom_seed))
      return -1;
    acquire(&urandom_lock);
    if(either_copyin(buf, 1, src, n) < 0){
      release(&urandom_lock);
      return -1;
    }
    memmove(&urandom_seed, buf, n);
    release(&urandom_lock);
    return n;
  case 3:  // /dev/nullstat
    acquire(&nullstat_lock);
    nullstat_count += n;
    release(&nullstat_lock);
    return n;
  default:
    return -1;
  }
}


struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
  initlock(&urandom_lock, "urandom");
  initlock(&nullstat_lock, "nullstat");

  devsw[PSEUDO_MAJOR].read  = pseudo_read;
  devsw[PSEUDO_MAJOR].write = pseudo_write;
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    uint dev = mkdev(f->major, f->minor);
    int maj = major(dev);
    if(maj < 0 || maj >= NDEV || !devsw[maj].read)
      return -1;
    r = devsw[maj].read(dev, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    int dev = mkdev(f->major, f->minor);
    int maj = major(dev);
    if(maj < 0 || maj >= NDEV || !devsw[maj].write)
      return -1;
    ret = devsw[maj].write(dev, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

