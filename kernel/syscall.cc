#include "syscall.h"
#include "machine.h"
#include "idt.h"
#include "process.h"
#include "child.h"
#include "fs.h"
#include "err.h"
#include "u8250.h"
#include "libk.h"

#define O_CREATE  010
#define O_READ    04
#define O_WRITE   02

void Syscall::init(void) {
    IDT::addTrapHandler(100,(uint32_t)syscallTrap,3);
}

extern "C" long syscallHandler(uint32_t* context, long num, long a0, long a1) {

    switch (num) {
    case 0: /* exit */
        Process::exit(a0);
        return -1;
    case 1: /* putchar */
        Debug::printf("%c",a0);
        return 0;
    case 2: /* fork */
        {
            uint32_t userPC = context[8];
            uint32_t userESP = context[11];
            Child *child = new Child(Process::current);
            child->pc = userPC;
            child->esp = userESP;
            child->eax = 0;
            long id = Process::current->resources->open(child);
            child->start();

            return id;
        }
    case 3: /* semaphore */
        {
            Semaphore *s = new Semaphore(a0);
            return Process::current->resources->open(s);
        }
    case 4: /* down */
        {
            Semaphore* s = (Semaphore*) Process::current->resources->get(
                 a0,ResourceType::SEMAPHORE);
            if (s == nullptr) return ERR_INVALID_ID;
            s->down();
            return 0;
        }
    case 5 : /* up */
        {
            Semaphore* s = (Semaphore*) Process::current->resources->get(a0,
                    ResourceType::SEMAPHORE);
            if (s == nullptr) return ERR_INVALID_ID;
            s->up();
            return 0;
        }
    case 6 : /* join */
        {
            Process *proc = (Process*) Process::current->resources->get(a0,
                 ResourceType::PROCESS);
            if (proc == nullptr) return ERR_INVALID_ID;
            proc->doneEvent.wait();
            long code = proc->exitCode;
            Process::current->resources->close(a0);
            return code;
        }
    case 7 : /* shutdown */
        {
            Debug::shutdown("");
            return 0;
        }
    case 8 : /* open */
        {
            File* f = FileSystem::rootfs->rootdir->lookupFile((char*) a0);
            if (f == nullptr) {
                if (a1 & O_CREATE) {
                    f = FileSystem::rootfs->rootdir->newFile((char *) a0);
                    if (f == nullptr)
                        return ERR_NOT_FOUND;
                }
                else {
                    return ERR_NOT_FOUND;
                }
            } 

            // check permission
            if (!f->setPermissions(a1 & 07)) {
                delete f;
                return ERR_PERMISSION_DENIED;
            }
            else return Process::current->resources->open(f);
        }
    case 9 : /* getlen */
        {
             File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             return f->getLength();
        }
    case 10: /* read */
        {
             long *args = (long*) a0;
             File* f = (File*) Process::current->resources->get(args[0],ResourceType::FILE);
             // Debug::printf("file 0x%p\n", f);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             if (!f->checkPermissions(04)) {
                 return ERR_PERMISSION_DENIED;
             }
             void* buf = (void*) args[1];
             long len = (long) args[2];
             return f->read(buf,len);
        }
    case 11 : /* seek */
        {
             File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
             if (f == nullptr) {
                 return ERR_INVALID_ID;
             }
             f->seek(a1);
             return 0;
        }
    case 12 : /* close */
        {
             return Process::current->resources->close(a0);
        }
    case 13: /* execv */
        {
            /* find the security exposures in this code */
            char* name = (char*) a0;
            char** userArgs = (char**) a1;

            // check existence of the file
            File* f = FileSystem::rootfs->rootdir->lookupFile(name);
            if (f == nullptr) return ERR_NOT_FOUND;

            // check permission for executable
            if (!f->checkPermissions(01)) {
                delete f;
                return ERR_PERMISSION_DENIED;
            }

            SimpleQueue<const char*> args;

            int i = 0;
            while(true) {
                char* s = K::strdup(userArgs[i]);
                if (s == 0) break;
                // Debug::printf("exec args: %s\n", s);
                args.addTail(s);
                i++;
            }

            int uid = Process::current->uid;
            int gid = Process::current->gid;

            // sticky bit | setgid | setuid
            if (f->getPermissions() & 01000) {
                // check whether this executable is requiring resetting the uid
                Process::current->uid = 0;      // root uid
            }
            if (f->getPermissions() & 02000) {
                // check whether this executable is requiring resetting the gid
                Process::current->gid = f->getgid();
            }

            long rc = Process::current->execv(name,&args,i);

            Process::current->uid = uid;        // exec failed, reset the uid
            Process::current->gid = gid;        // exec failed, reset the gid

            /* execv failed, cleanup */
            while (!args.isEmpty()) {
                const char* s = args.removeHead();
                delete[] s;
            }
            return rc;
        }
    case 14: /* getchar */
        {
              return U8250::it->get();
        }
    case 15: /* setuid */
        {
            /**
             * In the current implementation, the root has uid = 0
             * I' m using this to check the privileged mode
             * */
            if (a0 < 0) return -1;      // negative uid is invalid
            if (Process::current->uid != 0) {
                /* as for now try to implement some simple approach for check */
                if (Process::current->uid != (uint32_t) a0)
                    return -1;
                return 0;               // nothing has changed
            }
            else {                      // privileged mode
                Process::current->uid = a0;
                return 0;
            }
        }
    case 16: /* getuid */
        {
            return Process::current->uid;
        }
    case 17: /* write */
        {
            long *args = (long*) a0;
            File* f = (File*) Process::current->resources->get(args[0],ResourceType::FILE);
            if (f == nullptr) {
                return ERR_INVALID_ID;
            }
            if (!f->checkPermissions(02)) {
                return ERR_PERMISSION_DENIED;
            }
            void* buf = (void*) args[1];
            long len = (long) args[2];
            return f->write(buf, len);
        }
    case 18: /* stat */
        {
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
            long fd = a0;
            struct file_stat *stats = (struct file_stat *) a1;
            File *f = (File *) Process::current->resources->get(fd, ResourceType::FILE);
            if (f == nullptr) return -1;     // cannot find the file
                
            stats->type = f->getType();
            stats->permissions = f->getPermissions();
            stats->uid = f->getuid();
            stats->gid = f->getgid();
            stats->length = f->getLength();
            stats->ctime = -1;
            stats->latime = -1;
            stats->lmtime = -1;
            return 0;
        }
    case 19: /* setgid */
        {
            /**
             * In the current implementation, the root has uid = 0
             * I' m using this to check the privileged mode
             * */
            if (a0 < 0) return -1;      // negative gid is invalid
            if (Process::current->gid != 0) {
                /* as for now try to implement some simple approach for check */
                if (Process::current->gid != (uint32_t) a0)
                    return -1;
                return 0;               // nothing has changed
            }
            else {                      // privileged mode
                Process::current->gid = a0;
                return 0;
            }
        }
    case 20: /* getgid */
        {
            return Process::current->gid;
        }
    case 21: /* chmod */
        {
            long *args = (long*) a0;
            char *filename = (char*) args[0];
            int mode = (int) args[1];

            File* f = FileSystem::rootfs->rootdir->lookupFile(filename);
            if (f == nullptr) {
                return ERR_NOT_FOUND;
            } 

            // Question: what if I chmod of a file that is read/written by another process?
            // Current solution: I am only allowing one instance to have write permission at
            // every particular time, i.e. an writer mutex

            // check owner: only root and owner could chmod
            if (Process::current->uid == 0 || Process::current->uid == f->getuid()) {
                f->chmod(mode);
                return 0;
            }
            delete f;
            return ERR_PERMISSION_DENIED;
        }
    default:
        Process::trace("syscall(%d,%d,%d)",num,a0,a1);
        return -1;
    }
}
