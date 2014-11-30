#ifndef _SYS_H_
#define _SYS_H_

#include "stdint.h"

/**
 * Structure of FAT439File
 * Type                     32 bits
 * Permissions              32 bits
 * User ID                  32 bits
 * Group ID                 32 bits
 * Length                   32 bits
 * Creation Time            32 bits
 * Last Access Time         32 bits
 * Last Modification Time   32 bits
 * Deletion Time            32 bits
 **/
struct file_stat {
    uint32_t type;
    uint32_t permissions;
    uint32_t uid;
    uint32_t gid;
    uint32_t length;
    uint32_t ctime;
    uint32_t latime;
    uint32_t lmtime;
    uint32_t dtime;
};

#define O_CREATE  010
#define O_READ    04
#define O_WRITE   02

extern long exit(long status);
extern long execv(char* prog, char** args);
extern long open(char *name, long permissions);
extern long getlen(long);
extern long close(long);
extern long read(long f, void* buf, long len);
extern long write(long f, void* buf, long len);
extern long seek(long f, long pos);
extern long putchar(int c);
extern long getchar();
extern long semaphore(long n);
extern long up(long sem);
extern long down(long sem);
extern long fork();
extern long join(long proc);
extern long shutdown();
extern long setuid(long uid);
extern long getuid();
extern long fstat(long fd, struct file_stat *buf);
extern long setgid(long uid);
extern long getgid();
extern long chmod(char *filename, int mode);

#endif
