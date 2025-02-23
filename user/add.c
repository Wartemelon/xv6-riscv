#include "user/user.h"

int 
main(int argc, char *argv[]) 
{
    int num1, num2;
    char num1_str[256], num2_str[256];
    int i = 0, j = 0;
    int cc;
    char c;
    int is_num1_valid = 0, is_num2_valid = 0;

    cc = read(0, &c, 1);
    while (cc != 0) {
        if (cc < 1) {
            break;
        }
        if (i >= sizeof(num1_str) - 1) {
            printf("%s", "Buffer size exceeded\n");
            exit(1);
        }
        if (c == ' ' || c == '\n' || c == '\r') {
            is_num1_valid = 1;
            break;
        }
        if (c < '0' || c > '9') {
            fprintf(2, "Invalid number format\n");
            exit(1);
        }
        num1_str[i++] = c;
        cc = read(0, &c, 1);
    }
    if (is_num1_valid == 0) {
        fprintf(2, "Invalid number format\n");
        exit(1);
    }
    num1_str[i] = '\0';
    if (num1_str[0] == '\0') {
        fprintf(2, "Empty number\n");
        exit(1);
    }
    num1 = atoi(num1_str);

    cc = read(0, &c, 1);
    while (cc != 0) {
        if (cc < 1) {
            break;
        }
        if (j >= sizeof(num2_str) - 1) {
            printf("%s", "Buffer size exceeded\n");
            exit(1);
        }
        if (c == '\n' || c == '\r') {
            is_num2_valid = 1;
            break;
        }
        if (c < '0' || c > '9') {
            fprintf(2, "Invalid number format\n");
            exit(1);
        }
        num2_str[j++] = c;
        cc = read(0, &c, 1);
    }
    if (is_num2_valid == 0) {
        fprintf(2, "Invalid number format\n");
        exit(1);
    }
    num2_str[j] = '\0';
    if (num2_str[0] == '\0') {
        fprintf(2, "Empty number\n");
        exit(1);
    }
    num2 = atoi(num2_str);

    int sum = add(num1, num2); // Вызов системного вызова
    printf("%d\n", sum);

    //Test output
    printf("|%d %d|\n", num1, num2);
    // printf("num1: %d\n", num1);
    // printf("num2: %d\n", num2);
    exit(0);
}