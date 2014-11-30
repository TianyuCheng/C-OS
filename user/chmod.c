#include "err.h"
#include "libc.h"

int getPermission(char *p) {
    /* while (*p == '0') p++; */
    int length = strlen(p);
    if (length == 0) return 0;

    int permission = 0;
    int shift = 0;
    for (int i = length - 1; i >= 0; i--) {
        int bit = p[i] - '0';
        if (bit >= 8) return -1;
        permission += bit << shift;
        shift += 3;
    }
    return permission;
}

/**
 * This is a simplified version of chmod
 * */
int main(int argc, char** argv) {
    /* usage: chmod <mode> file */
    if (argc < 3) {
        puts("<usage>: chmod <mode> filename\n");
        return -1;
    }

    int permission = getPermission(argv[1]);
    char *filename = argv[2];

    if (permission < 0) {
        puts("mode is not in the correct format!\n");
        return -1;
    }

    long retval = chmod(filename, permission);

    if (retval < 0) {
        switch (retval) {
            case ERR_NOT_FOUND:
                puts("chmod file not found\n");
                break;
            case ERR_PERMISSION_DENIED:
                puts("chmod permission denied\n");
                break;
            default:
                puts("chmod error\n");
                break;
        }
    }

    return 0;
}
