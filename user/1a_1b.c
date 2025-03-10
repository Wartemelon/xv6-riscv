#include "user/user.h"

int task_1a() {
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        sleep(100);
        exit(1);
    }
    else {
        printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);
        int status;
        int waited_pid = wait(&status);
        printf("Child PID: %d exited with status: %d\n", waited_pid, status);
        exit(0);
    }
}

int task_1b() {
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        sleep(500);
        exit(1);
    }
    else {
        printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);
        if (kill(pid) < 0) {
            perror("kill failed");
            exit(1);
        }
        int status;
        int waited_pid = wait(&status);
        printf("Killed Child PID: %d exited with status %d\n", waited_pid, status);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Not correct number of arguments\n");
        exit(1);
    }
    if (strcmp(argv[1], "1a") == 0) {
        task_1a();
    }
    else if (strcmp(argv[1], "1b") == 0) {
        task_1b();
    }
    else {
        printf("Not correct argument\n");
        exit(1);
    }
    exit(0);
}