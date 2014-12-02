#include "fs.h"
#include "machine.h"
#include "process.h"
#include "stdint.h"
#include "err.h"

/**************/
/* FileSystem */
/**************/

FileSystem* FileSystem::rootfs = 0;

void FileSystem::init(FileSystem* rfs) {
    rootfs = rfs;
}

/*************************/
/* Fat439 implementation */
/*************************/

static uint32_t min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

/* An open Fat439 file, one per file, per system */
class OpenFile : public Resource {
/**
 * Structure of FAT439File
 * Type                     32 bits
 * Permissions              32 bits
 * User ID                  32 bits
 * Group ID                 32 bits
 * Length                   32 bits
 * Creation nime            32 bits
 * Last Access Time         32 bits
 * Last Modification Time   32 bits
 * Deletion Time            32 bits
 **/
public:
    uint32_t start;
    FileMetaData metaData;
    Fat439 *fs;
    Mutex writerMutex;

    OpenFile(Fat439 *fs, uint32_t start) : Resource(ResourceType::OTHER) {
        this->start = start;
        this->fs = fs;
        fs->dev->readFully(start * 512, &metaData, sizeof(metaData));
#if 0
        Debug::printf("type: %u\n", metaData.type);
        Debug::printf("permissions: %u\n", metaData.permissions);
        Debug::printf("userID: %u\n", metaData.userID);
        Debug::printf("groupID: %u\n", metaData.groupID);
        Debug::printf("length: %u\n", metaData.length);
        Debug::printf("creationTime: %u\n", metaData.creationTime);
        Debug::printf("lastAccessTime: %u\n", metaData.lastAccessTime);
        Debug::printf("lastModificationTime: %u\n", metaData.lastModificationTime);
        Debug::printf("deletionTime: %u\n", metaData.deletionTime);
#endif
    }

    virtual ~OpenFile() {
    }

    uint32_t getLength() { return  metaData.length; }
    uint32_t getType() { return metaData.type; }
    uint32_t getPermissions() { return metaData.permissions; }
    uint32_t getuid() {return metaData.uid; }
    uint32_t getgid() {return metaData.gid; }

    int32_t read(uint32_t offset, void* buf, uint32_t length) {
        if (offset > metaData.length) {
            return ERR_TOO_LONG;
        }
        uint32_t len = min(length, metaData.length - offset);

        uint32_t actualOffset = offset + sizeof(metaData);

        uint32_t blockInFile = actualOffset / 512;
        uint32_t offsetInBlock = actualOffset % 512;

        uint32_t blockNumber = start;

        for (uint32_t i=0; i<blockInFile; i++) {
            blockNumber = fs->fat[blockNumber];
        }

        int32_t count = fs->dev->read(blockNumber * 512 + offsetInBlock, buf, len);
        return count;
    }

    int32_t write(uint32_t offset, void* buf, uint32_t length) {
        if (offset > metaData.length) {
            return ERR_TOO_LONG;
        }
        // I should probably provide some check on arguments
        uint32_t actualOffset = offset + sizeof(metaData);

        uint32_t blockInFile = actualOffset / 512;
        uint32_t offsetInBlock = actualOffset % 512;

        uint32_t blockNumber = this->start;
        // Debug::printf("==> blockNumber: %u\n", blockNumber);
        
        // iterate to find the block for write
        for (uint32_t i=0; i<blockInFile; i++) {
            uint32_t nextBlock = fs->fat[blockNumber];
            if (!nextBlock) {
                if (!fs->super.avail) {
                    /* print the map */
                    for (uint32_t i = 0; i < fs->super.nBlocks; i++) {
                        if (i % 16 == 0)
                            Debug::printf("\n[%d]\t", i);
                        Debug::printf("%d\t", fs->fat[i]);
                    }
                    Debug::printf("\n");
                    Debug::panic("not enough disk space!");
                }
                // if we reach the end of the block list
                nextBlock = fs->super.avail;
                fs->super.avail = fs->fat[nextBlock];
                fs->fat[nextBlock] = 0;
                fs->fat[blockNumber] = nextBlock;
                // Debug::printf("==> blockNumber: %u\tnext avail: %u\n", blockNumber, fs->super.avail);
            }
            blockNumber = nextBlock;
        }
        // Debug::printf("--> blockNumber: %u\n", blockNumber);

        int32_t count = fs->dev->write(blockNumber * 512 + offsetInBlock, buf, length);

        // keep integrity of the file system
        uint32_t newLength = offset + count;
        // update metadata
        metaData.lmtime = -1;         // how should I get time?
        metaData.latime = -1;         // how should I get time?
        if (newLength < metaData.length) {
            // newLength is smaller than original length, therefore I need to mark the following blocks as free
            blockNumber = fs->fat[blockNumber];     // start to free from next
            if (blockNumber) {
                uint32_t tmp = fs->super.avail;
                fs->super.avail = blockNumber;      // prepend the remainings to the free list
                while (fs->fat[blockNumber]) {
                    blockNumber = fs->fat[blockNumber];
                }
                fs->fat[blockNumber] = tmp;
            }
        }

        metaData.length = newLength;
        fs->dev->write(start * 512, &metaData, sizeof(FileMetaData));
        return count;
    }

    int32_t readFully(uint32_t offset, void* buf, uint32_t length) {
        char* p = (char*) buf;
        uint32_t togo = length;
        while (togo) {
            int32_t cnt = read(offset,p,togo);
            if (cnt < 0) return cnt;
            if (cnt == 0) return length - togo;
            p += cnt;
            togo -= cnt;
            offset += cnt;
        }
        return length;
    }

    int32_t writeFully(uint32_t offset, void* buf, uint32_t length) {
        char* p = (char*) buf;
        uint32_t togo = length;
        // Debug::printf("write fully: %s\n", (char *)buf);
        while (togo) {
            int32_t cnt = write(offset,p,togo);
            if (cnt < 0) return cnt;
            if (cnt == 0) return length - togo;
            p += cnt;
            togo -= cnt;
            offset += cnt;
        }
        return length;
    }
};

class Fat439File : public File {
public:
    OpenFile *openFile;

    Fat439File(OpenFile* openFile) : File(openFile->fs) {
        this->openFile = openFile;
        this->permissions = openFile->metaData.permissions;
    }

    virtual ~Fat439File() {
        openFile->fs->closeFile(openFile);
    }

    virtual Fat439File* forkMe() {
        Resource::ref(openFile);
        Fat439File *other = new Fat439File(openFile);
        other->offset = offset;
        other->count.set(count.get());
        return other;
    }

    virtual uint32_t getLength() { return openFile->getLength(); }
    virtual uint32_t getType() { return openFile->getType(); }
    virtual uint32_t getPermissions() { return openFile->getPermissions(); }
    virtual uint32_t getuid() { return openFile->getuid(); }
    virtual uint32_t getgid() { return openFile->getgid(); }

    virtual void chmod(int mode) {
        openFile->writerMutex.lock();
        int permissions = openFile->metaData.permissions;
        openFile->metaData.permissions =  (permissions & ~0777) | (mode & 0777);
        // Debug::printf("mode: 0%o\n", mode & 0777);
        // Debug::printf("metaData: 0%o\n", openFile->metaData.permissions);
        fs->dev->write(openFile->start, &openFile->metaData, sizeof(FileMetaData));
        openFile->writerMutex.unlock();
    }

    virtual int32_t read(void* buf, uint32_t length) {
        long cnt = openFile->read(offset,buf,length);
        if (cnt > 0) offset += cnt;
        return cnt;
    }

    virtual int32_t write(void* buf, uint32_t length) {
        long cnt = openFile->write(offset,buf,length);
        if (cnt > 0) offset += cnt;
        return cnt;
    }

    virtual int setPermissions(int mask) {
        int retval = checkPermissions(mask);
        // check for writing permission
        // for simplicity it allows concurrent reads,
        // but not concurrent writes
        if (retval && permissions & 02) {
            openFile->writerMutex.lock();  // ask for write permission, therefore block
        }
        return retval;
    }
};

class Fat439Directory : public Directory {
    uint32_t start;
    OpenFile *content;
    uint32_t entries;
    Mutex mutex;
public:
    Fat439Directory(Fat439* fs, uint32_t start) : Directory(fs), start(start) {
        content = fs->openFile(start);
        entries = content->getLength() / 16;        
    }

    virtual ~Fat439Directory() {
        content->fs->closeFile(content);
    }

    uint32_t lookup(const char* name) {
        if ((name[0] == '.') && (name[1] == 0)) {
            return start;
        }
        struct {
            char name[12];
            uint32_t start;
        } entry;
        mutex.lock();
        uint32_t offset = 0;
        for (uint32_t i=0; i<entries; i++) {
            content->readFully(offset,&entry, 16);
            offset += 16;
            // Debug::printf("lookup <%s>\n", entry.name);
            for (int i=0; i<12; i++) {
                char x = entry.name[i];
                if (x != name[i]) break;
                if (x == 0) {
                    mutex.unlock();
                    return entry.start;
                }
            }
        }
        mutex.unlock();
        return 0;
    }

    File* lookupFile(const char* name) {
        uint32_t idx = lookup(name);
        if (idx == 0) return nullptr;

        return new Fat439File(content->fs->openFile(idx));
    }

    Directory* lookupDirectory(const char* name) {
        uint32_t idx = lookup(name);
        if (idx == 0) return nullptr;

        return new Fat439Directory(content->fs,idx);
    }

    File* newFile(const char* name) {
        mutex.lock();
        Fat439 *fat = reinterpret_cast<Fat439*>(fs);
        int fileStart = fat->super.avail;
        if (!fileStart) Debug::panic("not enough space on disk to create a new file!");

        fat->super.avail = fat->fat[fat->super.avail];
        fat->fat[fileStart] = 0;
        // Debug::printf("==>>>> next avail: %u\n", fat->super.avail);
        // Debug::printf("new file %s starts at %u\n", name, fileStart);

        // create meta data
        FileMetaData metaData;
        metaData.type = 1;
        metaData.permissions = 0644;    // default
        metaData.uid = Process::current->uid;
        metaData.gid = Process::current->gid;
        metaData.length = 0;
        // I have not implemented the time
        metaData.ctime = -1;
        metaData.latime = -1;
        metaData.lmtime = -1;
        metaData.dtime = -1;
        fs->dev->write(fileStart * 512, &metaData, sizeof(FileMetaData));

        OpenFile *openFile = fat->openFile(fileStart);
        Fat439File *file = new Fat439File(openFile);
        this->entries++;

        // update the directory entry
        int dirLength = content->getLength();
        char entry[16];
        for (int32_t i = 0; i < K::strlen(name); i++) {
            entry[i] = name[i];
            if (!entry[i]) break;
            entry[i + 1] = '\0';
        }
        // Debug::printf("create file length: %u <%s> <%s>\n", K::strlen(name), name, entry);
        uint32_t* firstBlock = (uint32_t *) &entry[12];
        *firstBlock = fileStart;
        content->writeFully(dirLength, (void*)entry, 16);

        // Debug::printf("dirLength: %u\n", content->getLength());

        mutex.unlock();
        return file;
    }
};

Fat439::Fat439(BlockDevice *dev) : FileSystem(dev) {
    dev->readFully(0, &super, sizeof(super));
    const uint32_t expectedMagic = 0x39333446;
    uint32_t magic;
    memcpy(&magic,super.magic,4);
    if (magic != 0x39333446) {
        Debug::panic("bad magic %x != %x",magic, expectedMagic);
    }

    fat = new uint32_t[super.nBlocks];
    openFiles = new OpenFilePtr[super.nBlocks]();
    dev->readFully(512,fat,super.nBlocks * sizeof(uint32_t));
    rootdir = new Fat439Directory(this,super.root);
}

OpenFile* Fat439::openFile(uint32_t start) {
    openFilesMutex.lock();
    OpenFile* p = openFiles[start];
    if (p == nullptr) {
        p = new OpenFile(this,start);
        openFiles[start] = p;
    }
    Resource::ref(p);
    openFilesMutex.unlock();
    return p;
}


void Fat439::closeFile(OpenFile* p) {
    openFilesMutex.lock();
    uint32_t start = p->start; 
    openFiles[start] = (OpenFile*) Resource::unref(openFiles[start]);
    openFilesMutex.unlock();

    // check whether there is some process waiting for writing this file
    OpenFile* after = openFiles[start];
    if (after && after->count.get() == 1 && after->writerMutex.isBlocked()) {
        after->writerMutex.unlock();
    }
}
