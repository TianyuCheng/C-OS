#include "err.h"
#include "libc.h"

void perror(int code) {
    if (code >= 0) return;

    switch (code) {
        case ERR_PERMISSION_DENIED:
            puts("permission denied\n");
            exit(-1);
            break;
        case ERR_NOT_FOUND:
            puts("file not found\n");
            exit(-1);
            break;
        default:
            puts("cp: error happened!\n");
            exit(-1);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        puts("<usage> cp <src> <dst>\n");
        return -1;
    }
    char *src = argv[1];
    char *dst = argv[2];

    long fsrc = open(src, O_READ);
    perror(fsrc);

    long fdst = open(dst, O_CREATE|O_WRITE);
    perror(fdst);

    long length = getlen(fsrc);

    char buffer[512];
    while (length > 0) {
        long cnt = readFully(fsrc, buffer, 512);
        if (cnt > 0) {
            length -= cnt;
            write(fdst, buffer, cnt);
        }
        else if (cnt == 0) {
            break;
        }
        else {
            puts("cp: error happened!\n");
            break;
        }
    }
    close(fsrc);
    close(fdst);
    return 0;
}
