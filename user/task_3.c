#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFF_SIZE 4096

ssize_t full_write(int fd, const void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t written = write(fd, (const char*)buf + total, count - total);
        if (written < 0) {
            return written;
        }
        total += written;
    }
    return total;
}

int main(int argc, char* argv[]) {
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        if (close(pipefd[1]) < 0) {
            perror("child: close(pipefd[1]) failed");
            exit(EXIT_FAILURE);
        }
        char buffer[BUFF_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, BUFF_SIZE)) > 0) {
            ssize_t total_written = 0;
            while (total_written < bytes_read) {
                ssize_t bytes_written = write(STDOUT_FILENO, buffer + total_written, bytes_read - total_written);
                if (bytes_written < 0) {
                    perror("child: write failed");
                    exit(EXIT_FAILURE);
                }
                total_written += bytes_written;
            }
        }
        if (bytes_read < 0) {
            perror("child: read failed");
            exit(EXIT_FAILURE);
        }
        if (close(pipefd[0]) < 0) {
            perror("child: close(pipefd[0]) failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        if (close(pipefd[0]) < 0) {
            perror("parent: close(pipefd[0]) failed");
            exit(EXIT_FAILURE);
        }
        char buffer[BUFF_SIZE];
        int offset = 0;
        for (int i = 1; i < argc; i++) {
            size_t len = strlen(argv[i]);
            /*
             * Если аргумент настолько длинный, что даже при пустом буфере
             * его длина с добавлением '\n' (len + 1) превышает BUFF_SIZE,
             * то перед записью сбрасываем накопленный буфер (если он не пуст)
             * и сразу записываем аргумент и символ новой строки.
             */
            if (len + 1 > BUFF_SIZE) {
                if (offset > 0) {
                    ssize_t written = full_write(pipefd[1], buffer, offset);
                    if (written != offset) {
                        perror("parent: write failed while flushing buffer");
                        close(pipefd[1]);
                        exit(EXIT_FAILURE);
                    }
                    offset = 0;
                }
                if (full_write(pipefd[1], argv[i], len) != len ||
                    full_write(pipefd[1], "\n", 1) != 1) {
                    perror("parent: write failed for long argument");
                    close(pipefd[1]);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            /*
             * Если текущий аргумент (с '\n') не помещается в оставшуюся часть буфера,
             * сбрасываем буфер в канал.
             */
            if (offset + len + 1 > BUFF_SIZE) {
                ssize_t written = full_write(pipefd[1], buffer, offset);
                if (written != offset) {
                    perror("parent: write failed while flushing buffer");
                    close(pipefd[1]);
                    exit(EXIT_FAILURE);
                }
                offset = 0;
            }
            // Копируем аргумент в буфер
            memcpy(buffer + offset, argv[i], len);
            offset += len;
            // Добавляем символ новой строки
            buffer[offset++] = '\n';
        }
        // Если в буфере ещё что-то осталось, сбрасываем его в канал
        if (offset > 0) {
            ssize_t written = full_write(pipefd[1], buffer, offset);
            if (written != offset) {
                perror("parent: write failed during final flush");
                close(pipefd[1]);
                exit(EXIT_FAILURE);
            }
        }

        if (close(pipefd[1]) < 0) {
            perror("parent: close(pipefd[1]) failed");
            exit(EXIT_FAILURE);
        }

        wait(0); 
    }

    return EXIT_SUCCESS;
}