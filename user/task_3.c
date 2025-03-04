#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFF_SIZE 4096

int main(int argc, char* argv[]) {
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        fprintf(stderr, "Pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(1);
    }

    if (pid == 0) {
        if (close(pipefd[1]) < 0) {
            fprintf(stderr, "Error: close(pipefd[1]) failed\n");
            exit(1);
        }

        char buffer[BUFF_SIZE];
        int bytes_read;

        while ((bytes_read = read(pipefd[0], buffer, BUFF_SIZE)) > 0) {
            int total_written = 0;
            while (total_written < bytes_read) {
                int bytes_written = write(1, buffer + total_written, bytes_read - total_written);
                if (bytes_written < 0) {
                    fprintf(stderr, "Error: write() failed\n");
                    exit(1);
                }
                total_written += bytes_written;
            }
        }

        if (close(pipefd[0]) < 0) {  
            fprintf(stderr, "Error: close(pipefd[0]) failed\n");
            exit(1);
        }
        exit(0);
    }
    else {
        if (close(pipefd[0]) < 0) {
            fprintf(stderr, "Error: close(pipefd[0]) failed\n");
            exit(1);
        }

        char buffer[BUFF_SIZE];
        int offset = 0;
        for (int i = 1; i < argc; i++) {
            int len = strlen(argv[i]);

            if (len >= BUFF_SIZE) {
                if (write(pipefd[1], argv[i], len) != len || write(pipefd[1], "\n", 1) != 1) {
                    fprintf(stderr, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                continue;
            }

            if (offset + len + 1 >= BUFF_SIZE) {
                if (write(pipefd[1], buffer, offset) != offset) {
                    fprintf(stderr, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                offset = 0;
            }
            memcpy(buffer + offset, argv[i], len);
            offset += len;

            if (offset + 1 >= BUFF_SIZE) {
                if (write(pipefd[1], buffer, offset) != offset) {
                    fprintf(stderr, "Error: write() failed\n");
                    close(pipefd[1]);
                    exit(1);
                }
                offset = 0;
            }
            buffer[offset++] = '\n';
        }

        if (offset > 0) {
            if (write(pipefd[1], buffer, offset) != offset) {
                fprintf(stderr, "Error: write() failed\n");
                close(pipefd[1]);
                exit(1);
            }
        }

        if (close(pipefd[1]) < 0) {
            fprintf(stderr, "Error: close(pipefd[1]) failed\n");
            exit(1);
        }

        wait(0); 
    }

    exit(0);
}