#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/procinfo.h"

int main(int argc, char *argv[]) {
    int count, ret;
    struct procinfo *buf;

    // Тест 1: Получение количества процессов
    count = ps_listinfo(0, 0);
    printf("Test 1: Process count = %d\n", count);

    // Тест 2: Попытка с недостаточным размером буфера
    int small_lim = count - 1;
    buf = malloc(small_lim * sizeof(struct procinfo));
    if (buf == 0) {
        printf("Test 2: Memory allocation failed for small buffer\n");
        exit(1);
    }
    ret = ps_listinfo(buf, small_lim);
    if (ret < 0) {
        printf("Test 2: Insufficient buffer test passed. Error code = %d\n", ret);
    }
    else {
        printf("Test 2: Insufficient buffer test failed. Retrieved %d entries, expected error.\n", ret);
    }
    free(buf);

    // Тест 3: Нормальный случай
    int lim = count;
    buf = malloc(lim * sizeof(struct procinfo));
    if (buf == 0) {
        printf("Test 3: Memory allocation failed for normal buffer\n");
        exit(1);
    }
    ret = ps_listinfo(buf, lim);
    if (ret < 0) {
        printf("Test 3: Normal test failed with error %d\n", ret);
    }
    else {
        printf("Test 3: Normal test succeeded. Retrieved %d entries.\n", ret);
        for (int i = 0; i < ret; i++) {
            printf("PID: %d, Name: %s, State: %d, PPID: %d, Parent: %s\n",
                   buf[i].pid, buf[i].name, buf[i].state, buf[i].ppid, buf[i].parent_name);
        }
    }
    free(buf);

    // Тест 4: Некорректный адрес
    // Передаём явно недопустимый адрес (например, 0x100)
    ret = ps_listinfo((struct procinfo *)0x100, count);
    if (ret < 0) {
        printf("Test 4: Invalid pointer test passed. Error code = %d\n", ret);
    }
    else {
        printf("Test 4: Invalid pointer test failed. Retrieved %d entries, expected error.\n", ret);
    }

    exit(0);
}
