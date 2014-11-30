#include "libc.h"
#include "err.h"

void cat(char *filename) {
    long file = open(filename, 04);

    // check whether the current directory exists
    if (file < 0) { 
        puts("cat: ");
        puts(filename);
        switch (file) {
            case ERR_NOT_FOUND:
                puts(": No such file or directory\n");
                break;
            case ERR_PERMISSION_DENIED:
                puts(": Permission denied\n");
                break;
            default:
                puts(": Some error happened\n");
        }
        return; 
    }

    char buffer[512];

    long length = getlen(file);
    /* puts("length of file: "); */
    /* putdec(length); */
    /* puts("\n"); */
    long togo = length;

    while (togo > 0) {
        int howMany = read(file, buffer, 511);
        if (howMany < 0) break;

        buffer[howMany] = '\0';
        puts(buffer);

        togo -= howMany;
    }

    close(file);
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        cat(argv[i]);
    }
    return 0;
}
