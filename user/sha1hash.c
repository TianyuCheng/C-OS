#include "libc.h"
#include "sha1.h"

int main(int argc, char** argv)
{ 
    if (argc < 1) {
        puts("usage: sha1hash <string>\n");
    }

    int length = 0;
    for (int i = 1; i < argc; i++) {
        length += strlen(argv[i]) + 1;
    }

    char *str = (char *) malloc(length);

    int index = 0;
    for (int i = 1; i < argc; i++) {
        memcpy(&str[index], argv[i], strlen(argv[i]));
        index += strlen(argv[i]) + 1;
        str[index - 1] = ' ';
    }

    str[length - 1] = '\0';

    puts("the string you want to hash is: ");
    puts(str);
    puts("\n");

    puts("the result of the string is: ");
    char *result = SHA1_Generate(str);
    puts(result);
    puts("\n");
    free(result);
    return 0;
}

