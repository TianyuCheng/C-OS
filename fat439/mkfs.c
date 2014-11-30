#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

typedef struct {
    char magic[4];
    uint32_t nBlocks;
    uint32_t avail;
    uint32_t root;
} Super;


struct metaData {
    uint32_t type;
    uint32_t permissions;       // sticky | setgid | setuid | rwx | rwx | rwx
    uint32_t uid;
    uint32_t gid;
    uint32_t length;
    uint32_t ctime;
    uint32_t latime;
    uint32_t lmtime;
    uint32_t dtime;
};

Super* super;
uint32_t *fat;
char* blocks;

void* mapStart;
size_t mapLength;

int nBlocks = 0;
int nFiles = 0;

uint32_t getBlock() {
    uint32_t idx = super->avail;
    if (idx == 0) {
        fprintf(stderr,"disk is full\n");
        exit(-1);
    }
    super->avail = fat[idx];
    fat[idx] = 0;
    return idx;
}

char* toPtr(uint32_t idx, uint32_t offset) {
    return blocks + idx * 512 + offset;
}

uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

uint32_t oneFile(const char* fileName, const int userID, const int groupID, const int permissions) {
    int fd = open(fileName,O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(-1);
    }

    uint32_t startBlock = getBlock();
    struct metaData *fileMetaData = (struct metaData *) toPtr(startBlock, 0);

    uint32_t currentBlock = startBlock;
    uint32_t leftInBlock = 512 - sizeof(struct metaData);
    uint32_t blockOffset = sizeof(struct metaData);
    uint32_t totalSize = 0;

    while (1) {
        if (leftInBlock == 0) {
            uint32_t b = getBlock();
            fat[currentBlock] = b;
            currentBlock = b;
            blockOffset = 0;
            leftInBlock = 512;
        }
        ssize_t n = read(fd,toPtr(currentBlock,blockOffset),leftInBlock);
        if (n < 0) {
            perror("read");
            exit(-1);
        } else if (n == 0) {
            /* eof */
            fileMetaData->type = 1;            // 1 for file
            fileMetaData->permissions = permissions;
            fileMetaData->uid = userID;
            fileMetaData->gid = groupID;
            fileMetaData->length = totalSize;

            struct timeval te; 
            gettimeofday(&te, NULL);
            long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
            fileMetaData->ctime = milliseconds;
            fileMetaData->latime = milliseconds;
            fileMetaData->lmtime = milliseconds;
            fileMetaData->dtime = milliseconds;
            break;
        } else {
            /* read some */
            blockOffset += n;
            leftInBlock -= n;
            totalSize += n;
        }
    }

    close(fd);

    return startBlock;
}

int readNumFiles(const char *conf) {
    FILE *fd = fopen(conf, "r");
    if (fd < 0) {
        perror("fopen");
        exit(-1);
    }
    char ch;
    int nLines = 0;
    do 
    {
        ch = fgetc(fd);
        if(ch == '\n')
            nLines++;
    } while (ch != EOF);
    return nLines;
}

void readConf(const char *conf) {
    FILE *fd = fopen(conf, "r");
    if (fd < 0) {
        perror("fopen");
        exit(-1);
    }
    int userID;
    int groupID;
    int permissions;
    char filename[200];

    /* root direcotry */
    super->root = getBlock();
    struct metaData *rootMetaData = (struct metaData *) toPtr(super->root,0);


    struct timeval te; 
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;

    rootMetaData->type = 2;
    rootMetaData->uid = 0;
    rootMetaData->gid = 0;
    rootMetaData->permissions = 0664;
    rootMetaData->length = nFiles * 16;
    rootMetaData->ctime = milliseconds;
    rootMetaData->latime = milliseconds;
    rootMetaData->lmtime = milliseconds;

    char* rootData = toPtr(super->root, sizeof(struct metaData));

    int index = 0;
    while (fscanf(fd, "%d %d %o %s", &userID, &groupID, &permissions, filename) != EOF) {
        printf("userID: %d\t", userID);
        printf("groupID: %d\t", groupID);
        printf("permissions: %o\t", permissions);
        printf("filename: %s\n", filename);

        uint32_t x = oneFile(filename, userID, groupID, permissions);
        char* nm = strdup(filename);
        char* base = basename(nm);
        char* dest = strncpy(rootData + index * 16, base, 12);
        free(nm);
        *((uint32_t*)(dest + 12)) = x;
        index++;
    }

    fclose(fd);
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"usage: %s <image name> <nBlocks> <conf-file> ...\n",argv[0]);
        exit(-1);
    }

    const char* imageName = argv[1];
    nBlocks = atoi(argv[2]);        // reading the number of blocks of this disk image
    nFiles = readNumFiles(argv[3]);

    int fd = open(imageName, O_CREAT | O_RDWR , 0666);
    if (fd == -1) {
        perror("create");
        exit(-1);
    }

    mapLength = nBlocks * 512;

    int rc = ftruncate(fd,mapLength);
    if (rc == -1) {
        perror("truncate");
        exit(-1);
    }

    mapStart = mmap(0,mapLength,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);
    if (mapStart == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    super = (Super*) mapStart;
    blocks = (char*) mapStart;
    fat = (uint32_t*) (blocks + 512);
    uint32_t fatBlocks = ((nBlocks * sizeof(uint32_t)) + 511) / 512;

    super->magic[0] = 'F';
    super->magic[1] = '4';
    super->magic[2] = '3';
    super->magic[3] = '9';
    super->nBlocks = nBlocks;
    super->avail = nBlocks - 1;

    uint32_t lastAvail = 1 + fatBlocks;

    for (uint32_t i=super->avail; i>lastAvail; i--) {
        fat[i] = i-1;
    }

    readConf(argv[3]);          // read and write

    munmap(mapStart,mapLength);

    return 0;
}
