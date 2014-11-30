#include "libc.h"

int main(int argc, char *argv[])
{
    long fd = -1;
    if (argc == 1) {
        fd = open("hello.txt", O_CREATE | O_READ | O_WRITE);
    }
    else {
        fd = open(argv[1], O_CREATE | O_READ | O_WRITE);
    }

    if (fd < 0) {
        puts("open file failed!\n");
        return -1;
    }

    char *buffer = "Hello, World! Writable file systems are cool!\n";

    /* puts("write length: "); */
    /* putdec(strlen(buffer) + 1); */
    /* puts("\n"); */

    for (int i = 0; i < 100; i++) {
        long n = writeFully(fd, buffer, strlen(buffer));
        if (n < 0) {
            puts("error after ");
            putdec(i);
            puts("times\n");
            break;
        }
    }

    /* puts("actual write length: "); */
    /* putdec(n); */
    /* puts("\n"); */

    close(fd);
    return 0;
}
