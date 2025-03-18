#include "kernel/types.h"
#include "kernel/procinfo.h"
#include "user/user.h"

static char *
procstate2str(int state) {
    switch (state)
    {
    case 0:
        return "UNUSED";
    case 1:
        return "SLEEP";
    case 2:
        return "RUNNABLE";
    case 3:
        return "RUNNING";
    case 4:
        return "ZOMBIE";
    }
    return "???";
}

int main(int argc, char *argv[]) {
    int n = ps_listinfo(0, 0);
    if (n < 0) {
        printf("ps: error %d\n", n);
        exit(1);
    }

    if (n == 0) {
        printf("No processes found.\n");
        exit(0);
    }

    struct procinfo *p = malloc(n * sizeof(struct procinfo));
    if (!p) {
        printf("ps: out of memory\n");
        exit(1);
    }

    int rc = ps_listinfo(p, n);
    if (rc < 0) {
        printf("ps_listinfo error %d\n", rc);
        free(p);
        exit(1);
    }

    printf("PID\tPPID\tSTATE\t\tNAME\tPARENT\n");

    for (int i = 0; i < rc; i++) {
        printf("%d\t%d\t%s\t%s\t%s\n",
               p[i].pid,
               p[i].ppid,
               procstate2str(p[i].state),
               p[i].name,
               p[i].parent_name);
    }

    free(p);
    exit(0);
}
