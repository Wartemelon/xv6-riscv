#include "user/user.h"

#define BUFF_SIZE 512

int main(int argc, char* argv[]) {
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        fprintf(2, "Pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "Fork failed\n");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(1);
    }

    if (pid == 0) {
        if (close(pipefd[1]) < 0) {
            fprintf(2, "Error: close(pipefd[1]) failed\n");
            exit(1);
        }

        close(0);
        if (dup(pipefd[0]) < 0) {
            fprintf(2, "Error: dup() failed\n");
            close(pipefd[0]);
            exit(1);
        }

        if (close(pipefd[0]) < 0) {  
            fprintf(2, "Error: close(pipefd[0]) failed\n");
            exit(1);
        }
        
        char *wc_argv[] = {"wc", 0};
        exec("wc", wc_argv);
        //Если exec() вернул управление, значит произошла ошибка
        fprintf(2, "Error: exec() failed\n");
        exit(1);
    }
    else {
        if (close(pipefd[0]) < 0) {
            fprintf(2, "Error: close(pipefd[0]) failed\n");
            exit(1);
        }

        char buffer[BUFF_SIZE];
        int offset = 0;
        for (int i = 1; i < argc; i++) {
            int len = strlen(argv[i]);

            if (len >= BUFF_SIZE) {
                if (write(pipefd[1], argv[i], len) != len || write(pipefd[1], "\n", 1) != 1) {
                    fprintf(2, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                continue;
            }

            if (offset + len + 1 >= BUFF_SIZE) {
                if (write(pipefd[1], buffer, offset) != offset) {
                    fprintf(2, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                offset = 0;
            }
            memcpy(buffer + offset, argv[i], len);
            offset += len;

            if (offset + 1 >= BUFF_SIZE) {
                if (write(pipefd[1], buffer, offset) != offset) {
                    fprintf(2, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                offset = 0;
            }
            buffer[offset++] = '\n';
        }

        if (offset > 0) {
            if (write(pipefd[1], buffer, offset) != offset) {
                fprintf(2, "Error: write() failed\n");
                close(pipefd[1]);
                exit(1);
            }
        }

        if (close(pipefd[1]) < 0) {
            fprintf(2, "Error: close(pipefd[1]) failed\n");
            exit(1);
        }

        wait(0); 
    }

    exit(0);
}