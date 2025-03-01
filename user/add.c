#include "user/user.h"

#define BUF_SIZE 256

int 
readNumber() {
    char buf[BUF_SIZE];
    int idx = 0;
    char c;
    int cc;

    while ((cc = read(0, &c, 1)) > 0) {
        if (c == ' ' || c == '\n') {
            break;
        }
        if (idx >= BUF_SIZE - 1) {
            fprintf(2, "Buffer size exceeded\n");
            exit(1);
        }
        if (c < '0' || c > '9') {
            fprintf(2, "Invalid number format\n");
            exit(1);
        }
        buf[idx++] = c;
    }

    if (cc < 0) {
        fprintf(2, "Error reading input\n");
        exit(1);
    }

    // Если мы вышли из цикла сразу, ничего не записав в buf (idx == 0),
    // значит пустой ввод или вообще не встретили цифр
    if (idx == 0) {
        fprintf(2, "Empty number\n");
        exit(1);
    }

    buf[idx] = '\0';
    return atoi(buf);
}

int 
main(int argc, char *argv[]) 
{
    int num1, num2;
    num1 = readNumber();
    num2 = readNumber();

    int sum = add(num1, num2); // Вызов системного вызова
    printf("%d\n", sum);

    //Test output
    printf("|%d %d|\n", num1, num2);
    // printf("num1: %d\n", num1);
    // printf("num2: %d\n", num2);
    exit(0);
}