#ifndef _FS_H_
#define _FS_H_

#include "libk.h"
#include "block.h"
#include "resource.h"
#include "semaphore.h"
#include "process.h"

/****************/
/* File systems */
/****************/

struct FileMetaData {
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

class FileSystem;

/* A file */
class File : public Resource {
public:
    FileSystem* fs;
    uint32_t offset;
    uint32_t permissions;

    File(FileSystem *fs) : Resource(ResourceType::FILE), fs(fs), offset(0) {}
    virtual ~File() {}

    /* We can ask it for its type */
    virtual uint32_t getType() = 0;    /* 1 => directory, 2 => file */
    /* We can ask it for its length in bytes */
    virtual uint32_t getLength() = 0;
    /* We can ask for its permission */
    virtual uint32_t getPermissions() = 0;
    /* We can get the User ID */
    virtual uint32_t getuid() = 0;
    /* We can get the Group ID */
    virtual uint32_t getgid() = 0;

    virtual void chmod(int mode) = 0;

    virtual void seek(uint32_t offset) {
        this->offset = offset;
    }

    /* We can read a few bytes, returned value:
         < 0 => error
         0 => end of file
         >0 => number of bytes read

         it's not an error to get less bytes than we've asked for
    */
    virtual int32_t read(void *buf, uint32_t length) = 0;

    /* read as many bytes as you can, returned value:

        < 0 => error
        n   => how many bytes were read

        n < length => end of file reached after n bytes
    */
    int32_t readFully(void* buf, uint32_t length) {
        uint32_t togo = length;
        char* ptr = (char*) buf;

        while (togo > 0) {
            int32_t c = read(ptr,togo);
            if (c < 0) return c;
            if (c == 0) return length - togo;
            togo -= c;
            ptr += c;
        }

        return length;
    }

    virtual int32_t write(void *buf, uint32_t length) = 0;


    virtual int checkPermissions(int mask) {

        if (mask == 0) return 1;        // require no permission

        int uid = Process::current->uid;
        int gid = Process::current->gid;
        int fileUID = this->getuid();
        int fileGID = this->getgid();
        int permissions = this->getPermissions();

        // owner
        if (uid == fileUID || uid == 0) {
            // Debug::printf("owner -> uid: %d\tfileUID: %u\tpermissions: %o\tmask:%o\n", uid, fileUID, (permissions >> 6) & 07, mask & 07);
            // Debug::printf("mask: %o =?= permissions: %o\n", (mask & 07), mask & (permissions >> 6) & 07);
            return (mask & (permissions >> 6) & 07) == (mask & 07);
        }

        // same group
        if (gid == fileGID) {
            // Debug::printf("group -> gid: %d\tfileGID: %u\tpermissions: %o\tmask:%o\n", gid, fileGID, (permissions >> 3) & 07, mask & 07);
            // Debug::printf("mask: %o =?= permissions: %o\n", (mask & 07), mask & (permissions >> 3) & 07);
            return (mask & (permissions >> 3) & 07) == (mask & 07);
        }

        // Debug::printf("other -> gid: %d\tfileGID: %u\tpermissions: %o\tmask:%o\n", gid, fileGID, (permissions >> 3) & 07, mask & 07);
        // Debug::printf("mask: %o =?= permissions: %o\n", (mask & 07), mask & (permissions) & 07);
        // other groups
        return (mask & permissions & 07) == (mask & 07);
    }

    virtual int setPermissions(int mask) {
        int retval = checkPermissions(mask);
        if (retval) {
            permissions = mask;
        }
        return retval;
    }
};

/* A directory */
class Directory {
public:
    FileSystem* fs;
    Directory(FileSystem *fs) : fs(fs) {}
    virtual File* lookupFile(const char* name) = 0;
    virtual Directory* lookupDirectory(const char *name) = 0;
    virtual File* newFile(const char* name) = 0;
};

class FileSystem {
public:
     static FileSystem *rootfs;    // the root file system
     static void init(FileSystem *rfs);

     BlockDevice *dev;
     Directory *rootdir;           // the root directory

     FileSystem(BlockDevice *dev) : dev(dev) {}
};

/**************************/
/* The FAT439 file system */
/**************************/

class OpenFile;
typedef OpenFile *OpenFilePtr;

class Fat439 : public FileSystem {
public:
    struct {
        char magic[4];
        uint32_t nBlocks;
        uint32_t avail;
        uint32_t root;
    } super;
    uint32_t *fat;
    Mutex openFilesMutex;
    OpenFilePtr *openFiles;
    Fat439(BlockDevice *dev);
    OpenFile* openFile(uint32_t start);
    void closeFile(OpenFile* of);
};

#endif
