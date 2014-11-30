REPORT
======

TARGET
------
+ adding current uid to process
+ adding a simple login process
+ adding permissions to files
+ adding logout commands

2014-11-21
----------

+ Modify the file system so that the each file contains userID, groupID,
  permission, creation time, last access time, and last modification time.
  - now only using uid
  - leave everything else to be implemented

+ Add buffered input
  - support backspace
  - does not support arrow keys

+ Add login process
  - setuid() using some simple approach to check for security
    + only root could modify its uid
    + non-privileged process could only set the same uid
  - getuid() 

+ Add "exit" command to shell

+ Add SHA1 as the one-way hashing function for password
  + Source:
    - http://svn.ghostscript.com/jbig2dec/trunk/sha1.h
    - http://svn.ghostscript.com/jbig2dec/trunk/sha1.c

2014-11-22
----------
+ Add ls -l functionality

+ Add simple access control
  - support individual users
  - support group


2014-11-25
----------
+ Writable File System

2014-11-26
----------
+ Add setgid | setuid bits in the access control list
+ Add su
+ Enable creating new file from open syscall
+ Add chmod
+ Add cp
