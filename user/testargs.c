#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

void
print_args(int argc, char *argv[], int use_mutex, int mfd)
{
  char tmp[2];
  tmp[1] = '\0';
  for(int i = 1; i < argc; i++){
    for(int j = 0; argv[i][j] != '\0'; j++){
      if(use_mutex){
        if(mutex_lock(mfd) < 0){
          printf("error: mutex_lock failed\n");
          exit(1);
        }
      }
      tmp[0] = argv[i][j];
      printf("%d: arg %d, char '%s'\n", getpid(), i, tmp);
      sleep(1);

      if(use_mutex){
        if(mutex_unlock(mfd) < 0){
          printf("error: mutex_unlock failed\n");
          exit(1);
        }
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  printf("=== Without mutex ===\n");

  int pid = fork();
  if(pid < 0){
    printf("fork error\n");
    exit(1);
  }
  if(pid == 0){
    print_args(argc, argv, 0, -1);
    exit(0);
  } else {
    print_args(argc, argv, 0, -1);
    wait((int*)0);
  }

  printf("\n=== With mutex ===\n");
  
  int m = mutex();
  if(m < 0){
    printf("cannot create mutex\n");
    exit(1);
  }

  pid = fork();
  if(pid < 0){
    printf("fork error\n");
    exit(1);
  }
  if(pid == 0){
    print_args(argc, argv, 1, m);
    exit(0);
  } else {
    print_args(argc, argv, 1, m);
    wait((int*)0);
    close(m);
  }

  exit(0);
}
