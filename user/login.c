#include "libc.h"
#include "sha1.h"
#include "err.h"

char *hostname = 0;

void printLogo() {
    puts("     .--.      \n"); 
    puts("    |o_o |     ");    puts("  ██████╗ ██████╗ ███████╗\n"); 
    puts("    |:_/ |     ");    puts(" ██╔════╝██╔═══██╗██╔════╝\n"); 
    puts("   //   \\ \\    ");  puts(" ██║     ██║   ██║███████╗\n"); 
    puts("  (|     | )   ");    puts(" ██║     ██║   ██║╚════██║\n"); 
    puts(" /'\\_   _/`\\   ");  puts(" ╚██████╗╚██████╔╝███████║\n"); 
    puts(" \\___)=(___/   ");   puts("  ╚═════╝ ╚═════╝ ╚══════╝\n"); 
    puts("\n");
}

char *getPassword() {
    int sz = 0;
    int i = 0;
    char *p = 0;

    while (1) {
        char c = getchar();
        switch (c) {
            case 13:
                {
                    if (i >= sz) {
                        sz += 10;
                        p = (char *) realloc(p, sz + 1);
                        if (p == 0) return 0;
                    }
                    p[i++] = '\0';
                    putchar('\n');
                    return p;
                }
            case 127:
                {
                    if (i >= 1) { i--; }
                    break;
                }
            default:
                {
                    if (i >= sz) {
                        sz += 10;
                        p = (char *) realloc(p, sz + 1);
                        if (p == 0) return 0;
                    }
                    p[i++] = c;
                }
        }
    }
    return p;
}

char *getHostname() {
    long fd = open("hosts", 04);
    if (fd < 0) {
        if (fd == ERR_PERMISSION_DENIED) {
            puts("hostnames permission denied\n");
        }
        else {
            puts("failed to open hostnames\n");
        }
        return 0;
    }

    long length = getlen(fd);
    char *hostname = (char *) malloc(length);
    readFully(fd, hostname, length);

    close(fd);
    hostname[strlen(hostname) - 1] = '\0';
    return hostname;
}

/**
 * return uid and gid
 * */
void verifyUser(char *username, char *password, char *shell, int id[2]) {
    /* open passwd to check */
    long fd = open("passwd", 04);
    
    long length = getlen(fd);
    char *buffer = (char *) malloc(length);
    readFully(fd, buffer, length);
    int tokens = strtok(buffer, ':');
        
    long tmp = (long) buffer;
    /* check whether there are enough tokens*/
    if (tokens < 4) {
        free(buffer);
        close(fd);
        return;
    }

    char* info[5];       /* username:password:uid:gid:shell */
    int uid = -1;
    int gid = -1;
    /* iterate through the list of users to verify */
    while (tokens >= 5) {
        info[0] = (char *) tmp; tmp += strlen(info[0]) + 1;
        info[1] = (char *) tmp; tmp += strlen(info[1]) + 1;
        info[2] = (char *) tmp; tmp += strlen(info[2]) + 1;
        info[3] = (char *) tmp; tmp += strlen(info[3]) + 1;
        info[4] = (char *) tmp; tmp += strlen(info[4]) + 1;
        tokens -= 5;
#if 0
        puts("uid: ");      puts(info[2]); puts("\t");
        puts("gid: ");      puts(info[3]); puts("\t");
        puts("username: "); puts(info[0]); puts("\t");
        puts("password: "); puts(info[1]); puts("\t");
        puts("shell: ");    puts(info[4]); puts("\n");
#endif
        if (strcmp(info[0], username) == 0 && SHA1_Verify(info[1], password)) {
            // matches, verification success
            uid = atoi(info[2]);
            gid = atoi(info[3]);
            memcpy(shell, info[4], strlen(info[4]) + 1);
            break;
        }
    }

    free(buffer);
    close(fd);

    id[0] = uid;
    id[1] = gid;
}

int login(long uid, long gid, char *username, char *shell) {
    long fid = fork();
    if (fid == 0) {
        /* child */
        long ret1 = setuid(uid);        // every child of this shell will be of this uid
        long ret2 = setgid(gid);
        if (ret1 >= 0 && ret2 >= 0) {
            char *args[3] = {shell};
            execv(shell, args);
        }
        else {
            puts("setuid/setgid failed! Please check whether you are in privileged mode\n");
        }
    } else {
        /* parent */
        join(fid);      // waiting for child to exit
        printLogo();
    }
    return 0;
}

int main(int argc, char **argv) {

    printLogo();

    char *username = 0;
    char *password = 0;

    hostname = getHostname();           // currently this is never free'd because I never found the way to exit this program

    while (1) {
        if (username) free(username);
        if (password) free(password);

        puts("\n");
        /* get the username */
        // showing login prompt
        puts(hostname);
        puts(" login: ");
        username = gets();

        /* get the password */
        // showing login prompt
        puts("passwd: ");
        password = getPassword();

        if (!username || !*username || !password || !*password) {
            /* empty username or password */
            puts("incorrect username/password. please try again\n");
            continue;
        }

        char shell[100];
        int id[2];
        verifyUser(username, password, shell, id);
        if (id[0] >= 0 && id[1] >= 0) {
            login(id[0], id[1], username, shell);
        }
        else {
            puts("incorrect username/password. please try again\n");
        }

        /* clean up the username and password buffer */
        free(username);
        free(password);
        username = 0;
        password = 0;
    }
    return 0;
}
