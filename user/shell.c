#include "libc.h"
#include "sys.h"

char *username = 0;
char *hostname = 0;

char* getHostname() {
    long fd = open("hosts", 04);
    if (fd < 0) {
        puts("failed to open hostnames");
        return 0;
    }

    long length = getlen(fd);
    char *hostname = (char *) malloc(length);
    readFully(fd, hostname, length);

    close(fd);
    hostname[strlen(hostname) - 1] = '\0';
    return hostname;
}

void notFound(char* cmd) {
    puts(cmd);
    puts(": command not found\n");
}

/**
 * return uid
 * */
char *getCurrentUser(int uid) {
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

char **makeArgs(unsigned argc, char *cmd) {
    int argNum = argc + 1;
    char **argv = (char **) malloc(sizeof(char*) * argNum) ;
    for (int i = 0; i < argNum - 1; i++) {
        char *arg = (char*) cmd;
        while (*cmd++); 
        argv[i] = arg;
    }
    argv[argNum - 1] = 0;         // terminator
#if 0
    for (int i = 0; i < argc; i++) {
        char * arg = argv[i];
        puts("arg: ");
        puts(arg);
        puts("\n");
    }
#endif
    return argv;
}

/**
 * return 0 for not-existing files
 * return 1 for executable
 * */
int getFileType(char *filename) {
    long file = open(filename, 01);
    if (file < 0) {
        return 0;   // not finding the file
    }

    /* // reading file identifier */
    /* char buf[4]; */
    /* read(file, buf, 4); */
    /* close(file); */
    /*  */
    /* // check ELF identifier */
    /* if (buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') */
    /*     return 1;       // ELF */
    /* return 2;           // file */
    return 1;
}

void exec(char *filename, char **argv) {
    long fid = fork();
    if (fid == 0) {
        /* child */
        execv(filename, argv);
    } else {
        /* parent */
        join(fid);
    }
}

int builtin(char *cmd) {
    if (strcmp("exit", cmd) == 0) {
        free(username);
        free(hostname);
        exit(0);
        return 1;
    }
    return 0;
}

void executeCommands(char *in) {
    int tokens = strtok(in, ' ');              // preprocessing the command
    if (tokens >= 1) {
        char **argv = makeArgs(tokens, in);   // turn into arguemnt array

        // check whether this is a built-in cmd
        if (!builtin(argv[0])) {

            // now verify that it is not a built-in cmd
            int filetype = getFileType(argv[0]);
            switch (filetype) {
                case 0:  /* not a file */
                    {
                        notFound(argv[0]);
                        break;
                    }
                case 1: /* an executable */
                    {
                        exec(in, argv);
                        break;
                    }
                /* case 2: #<{(| a file |)}># */
                /*     { */
                /*         char *newArgv[3]; */
                /*         newArgv[0] = "cat"; */
                /*         newArgv[1] = argv[0]; */
                /*         newArgv[2] = 0; */
                /*         exec("cat", newArgv); */
                /*         break; */
                /*     } */
            }

        }
        free(argv); // already used, so delete
    }
    free(in);       // already used, so delete
}

int main(int argc, char **argv) {

    username = getCurrentUser(getuid());
    hostname = getHostname();

    while (1) {
        puts(username);
        puts("@");
        puts(hostname);
        puts(":/$ ");
        char* in = gets();
        
        // check if really gets input
        if (in && *in) {
            executeCommands(in);
        }
    }
    return 0;
}
