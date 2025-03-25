#ifndef MUTEX_H
#define MUTEX_H

struct mutex {
  struct sleeplock lock;
  int owner;
};

struct file* mutexalloc(void);
void mutexclose(struct file *m);

#endif
