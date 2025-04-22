#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(2, "usage: %s <file> <numbytes>\n", argv[0]);
        exit(1);
    }

    char *path = argv[1];
    int num = atoi(argv[2]);
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        fprintf(2, "hexdump: cannot open %s\n", path);
        exit(1);
    }

    unsigned char buf[num];
    int r = read(fd, buf, num);
    if(r <= 0) {
        close(fd);
        exit(0);
    }
    for(int i = 0; i < r; i++){
        if(buf[i] < 16)
            printf("0");
        printf("%x ", buf[i]);
    }
    printf("\n");
    close(fd);
    exit(0);
}
