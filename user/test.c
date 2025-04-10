#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGINFO_FL_D 0x1
#define PGINFO_FL_A 0x2

#define PGFLAGCLEAR_D 0x1
#define PGFLAGCLEAR_A 0x2

static int gvar = 123;

int
main(int argc, char *argv[])
{
  printf("\n=== [1] Page table at program start (all pages) ===\n");
  pgtableinfo(0, 0, 0);
  int localvar = 456;
  #define STKSZ 512
  int stackarr[STKSZ];
  for (int i = 0; i < STKSZ; i++) {
    stackarr[i] = i;
  }
  int npages = 3;
  int size = npages * 4096;
  char *heaparr = sbrk(size);
  if ((void*)heaparr == (void*)-1) {
    printf("sbrk failed to allocate %d bytes\n", size);
    exit(1);
  }
  for (int i = 0; i < size; i++) {
    heaparr[i] = (char)(i & 0xFF);
  }
  printf("\n=== [2] After allocating %d pages (total %d bytes) ===\n", npages, size);
  pgtableinfo(0, 0, 0);
  printf("\n=== [3] Clear bits A and D for ALL pages ===\n");
  /*
  * Тут стоит заметить одну интересную вещь при выводе - повторное появление флагов A и D.
  * Для себя я это объясняю так:
  * даже после вызова clr_pgflags и сброса битов A и D,
  * в момент следующих обращений к памяти (например, при выполнении printf или других операций)
  * процессор автоматически устанавливает флаг A, а при записи – флаг D.
  * То есть, если между вызовом clr_pgflags и выводом информации происходит обращение к памяти,
  * аппарат сразу же заново устанавливает эти флаги
  */
  clr_pgflags(0, 0, PGFLAGCLEAR_A | PGFLAGCLEAR_D);
  pgtableinfo(0, 0, 0);
  int sum = gvar + localvar + stackarr[10] + (int)heaparr[5];
  printf("\nRead sum = %d\n", sum);
  printf("\n=== [4] Pages with A or D set after reading ===\n");
  pgtableinfo(0, 0, PGINFO_FL_A | PGINFO_FL_D);
  gvar++;
  localvar--;
  stackarr[10] += 777;
  heaparr[5] = 77;
  printf("\n=== [5] Pages with A or D set after writing ===\n");
  pgtableinfo(0, 0, PGINFO_FL_A | PGINFO_FL_D);
  sbrk(-size);
  printf("\n=== [6] After freeing the allocated heap pages ===\n");
  pgtableinfo(0, 0, 0);
  printf("\n=== End of test ===\n");
  exit(0);
}
