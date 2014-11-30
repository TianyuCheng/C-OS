#include "libc.h"

int flagL = 0;

char *getUser(int uid) {
    /* open passwd to check */
    long fd = open("passwd", 04);

    if (fd < 0) {
        puts("ls: failed to open passwd\n");
        return 0;
    }
    
    long length = getlen(fd);
    char *buffer = (char *) malloc(length);
    readFully(fd, buffer, length);
    int tokens = strtok(buffer, ':');
        
    long tmp = (long) buffer;
    /* check whether there are enough tokens*/
    if (tokens < 5) {
        free(buffer);
        close(fd);
        return 0;
    }

    char *username = 0;
    char* info[5];       /* username:password:uid:gid:shell */
    /* iterate through the list of users to verify */
    while (tokens >= 5) {
        info[0] = (char *) tmp; tmp += strlen(info[0]) + 1;
        info[1] = (char *) tmp; tmp += strlen(info[1]) + 1;
        info[2] = (char *) tmp; tmp += strlen(info[2]) + 1;
        info[3] = (char *) tmp; tmp += strlen(info[3]) + 1;
        info[4] = (char *) tmp; tmp += strlen(info[4]) + 1;
        tokens -= 5;

        if (atoi(info[2]) == uid) {
            int length = strlen(info[0]) + 1;
            username = (char *) malloc(length);
            memcpy(username, info[0], length);
            break;
        }
    }

    free(buffer);
    close(fd);

    return username;
}

char *getGroup(int gid) {
    /* open passwd to check */
    long fd = open("group", 04);

    if (fd < 0) {
        puts("ls: failed to open group\n");
        return 0;
    }
    
    long length = getlen(fd);
    char *buffer = (char *) malloc(length);
    readFully(fd, buffer, length);
    int tokens = strtok(buffer, ':');
        
    long tmp = (long) buffer;
    /* check whether there are enough tokens*/
    if (tokens < 3) {
        free(buffer);
        close(fd);
        return 0;
    }

    char *groupname = 0;
    char* info[3];       /* groupname:password:gid*/
    /* iterate through the list of users to verify */
    while (tokens >= 3) {
        info[0] = (char *) tmp; tmp += strlen(info[0]) + 1;
        info[1] = (char *) tmp; tmp += strlen(info[1]) + 1;
        info[2] = (char *) tmp; tmp += strlen(info[2]) + 1;
        tokens -= 3;

        if (atoi(info[2]) == gid) {
            int length = strlen(info[0]) + 1;
            groupname = (char *) malloc(length);
            memcpy(groupname, info[0], length);
            break;
        }
    }

    free(buffer);
    close(fd);

    return groupname;
}

void printPermissions(unsigned int permissions) {
    char info[11] = "-rwxrwxrwx";
    int i = 9;
    while (permissions > 0 ) {
        int bit = permissions & 0x1;
        permissions >>= 1;
        if (!bit)
            info[i] = '-';
        i--;
    }
    while (i >= 0) {
        info[i] = '-';
        i--;
    }
    puts(info);
}

int contains(char *arg, char ch) {
    int i = 0;
    while (arg[i]) {
        if (arg[i] == ch) return 1;
        i++;
    }
    return 0;
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            puts("usage: ls [-l]\n");
            return -1;
        }
        if (contains(argv[i], 'l'))
            flagL = 1;
    }

    long file = open(".", 04);

    // check whether the current directory exists
    if (file < 0) { return 1; }
    
    char buffer[16];

    struct file_stat stats;
    while (read(file, buffer, 16) == 16) {
        buffer[12] = '\0';
        if (flagL) {
            /* list mode*/
            long fd = open(buffer, 00);
            if (fd < 0) {
                puts("filesystem corrupted: file ");
                puts(buffer);
                puts(" has not been found!\n");
                return 0;
            }
            fstat(fd, &stats);
            printPermissions(stats.permissions);
            puts(" ");

            /* show user owner */
            char *username = getUser(stats.uid);
            puts(username);
            if (strlen(username) <= 4) puts("\t\t");
            else puts("\t");
            free(username);

            /* show group owner */
            char *groupname = getGroup(stats.gid);
            puts(groupname);
            /* puts(" "); */
            /* puts("gid");  // should print group as well, but we don't have groups */
            if (strlen(groupname) <= 4) puts("\t\t");
            else puts("\t");
            free(groupname);

            /* show file size */
            if (stats.length < 1024) {
                putdec(stats.length);
            }
            else {
                putdec(stats.length / 1024);
                puts("K");
            }

            /* puts(" "); */
            /* puts(" last access time "); */

            puts("\t");
            puts(buffer);
            puts("\n");
            close(fd);
        }
        else {
            puts(buffer);
            puts("\n");
        }
    }

    close(file);
    return 0;
}
