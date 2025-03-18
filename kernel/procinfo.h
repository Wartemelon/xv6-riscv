#ifndef PROCINFO_H
#define PROCINFO_H

#define PROC_NAME_LEN 16

struct procinfo {
  int pid;
  char name[PROC_NAME_LEN];
  int state;
  int ppid;
  char parent_name[PROC_NAME_LEN];
};

#endif // PROCINFO_H