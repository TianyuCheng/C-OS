#include "child.h"
#include "machine.h"
#include "err.h"

Child::Child(Process *parent) : Process("child",parent->resources->forkMe()) {
    parent->addressSpace.fork(&addressSpace);
    uid = parent->uid;          // set the uid to be the same as parent
    gid = parent->gid;          // set the gid to be the same as parent
}

long Child::run() {
    switchToUser(pc,esp,0);
    return ERR_NOT_POSSIBLE;
}
