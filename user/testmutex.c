#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

void test_read_write(int fd)
{
  char buf[10];
  int r = read(fd, buf, 10);
  printf("read(fd=%d) returned %d\n", fd, r);
  r = write(fd, buf, 5);
  printf("write(fd=%d) returned %d\n", fd, r);
}

void test_close_locked_by_self(void)
{
  printf("test_close_locked_by_self\n");

  int m = mutex();
  if(m < 0){
    printf("cannot create mutex\n");
    return;
  }
  if(mutex_lock(m) < 0){
    printf("lock failed\n");
  }
  close(m);
}

void test_close_locked_by_other(void)
{
  printf("test_close_locked_by_other\n");

  int m = mutex();
  if(m < 0){
    printf("cannot create mutex\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("fork error\n");
    return;
  }

  if(pid == 0){
    mutex_lock(m);
    printf("child locked m, going to sleep...\n");
    sleep(50);
    printf("child exit\n");
    exit(0);
  } else {
    sleep(10);
    printf("parent: try close(m)\n");
    close(m); 
    wait((int*)0);
  }
}

void test_exit_while_locked(void)
{
  printf("test_exit_while_locked\n");

  int m = mutex();
  if(m < 0){
    printf("cannot create mutex\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("fork error\n");
    return;
  }

  if(pid == 0){
    mutex_lock(m);
    printf("child locked m, but will exit now...\n");
    exit(0);
  } else {
    wait((int*)0);
    printf("parent: child exited. we expect the kernel to close m.\n");
  }
}

void test_unlock_foreign(void)
{
  printf("test_unlock_foreign\n");

  int m = mutex();
  if(m < 0){
    printf("cannot create mutex\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("fork error\n");
    return;
  }

  if(pid == 0){
    printf("child lock(m)\n");
    mutex_lock(m);
    sleep(50);
    printf("child exit without unlock\n");
    exit(0);
  } else {
    sleep(20);
    printf("parent tries to unlock(m)...\n");
    int r = mutex_unlock(m);
    printf("mutex_unlock returned %d\n", r);
    wait((int*)0);
    close(m);
  }
}

int
main(int argc, char *argv[])
{
  printf("1) Checking read/write on mutex:\n");
  int m = mutex();
  test_read_write(m);
  close(m);

  test_close_locked_by_self();

  test_close_locked_by_other();

  test_exit_while_locked();

  test_unlock_foreign();

  printf("All tests done.\n");
  exit(0);
}
