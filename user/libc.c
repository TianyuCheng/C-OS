#include "libc.h"
#include "sys.h"

static char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

void puts(char* p) {
    char c;
    while ((c = *p++) != 0) putchar(c); 
}

char *gets() {
    int sz = 0;
    int i = 0;
    char *p = 0;

    while (1) {
        char c = getchar();
        switch (c) {
            /* case 37: */
            /* case 38: */
            /* case 39: */
            /* case 40: */
            /*     { */
            /*         // not doing anything with arrow keys */
            /*         break; */
            /*     } */
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
                    if (i >= 1) {
                        i--;
                        puts("\b \b");
                    }
                    break;
                }
            default:
                {
                    putchar(c);
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
        
void puthex(long v) {
    for (int i=0; i<sizeof(long)*2; i++) {
          char c = hexDigits[(v & 0xf0000000) >> 28];
          putchar(c);
          v = v << 4;
    }
}

void putdec(unsigned long v) {
    if (v >= 10) {
        putdec(v / 10);
    }
    putchar(hexDigits[v % 10]);
}

long readFully(long fd, void* buf, long length) {
    long togo = length;
    char* p = (char*) buf;
    while(togo) {
        long cnt = read(fd,p,togo);
        if (cnt < 0) return cnt;
        if (cnt == 0) return length - togo;
        p += cnt;
        togo -= cnt;
    }
    return length;
}

long writeFully(long fd, void* buf, long length) {
    long togo = length;
    char* p = (char*) buf;
    while(togo) {
        long cnt = write(fd,p,togo);
        if (cnt < 0) return cnt;
        if (cnt == 0) return length - togo;
        p += cnt;
        togo -= cnt;
    }
    return length;
}

// source: http://opensource.apple.com/source/Libc/Libc-262/ppc/gen/strcmp.c
int strcmp(char *s1, char *s2) {
    for ( ; *s1 == *s2; s1++, s2++)
	if (*s1 == '\0')
	    return 0;
    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
}


int trimWord(char *cmd, char target) {
    // check whether the string is empty
    if (cmd[0] == '\0') return 0;

    int offset = 0;
    int index = 0;
    int retval = 0;

    // trim front
    while (cmd[offset] == target) offset++;
    if (offset) {
        // copy the word to the frond and move the index to next word
        while (cmd[offset] != target && cmd[offset] != '\0' && cmd[offset] != '\n') {
            cmd[index++] = cmd[offset];
            cmd[offset++] = ' ';
        }
    }
    else {
        // move the index to the next word
        while (cmd[offset] != target && cmd[offset] != '\0' && cmd[offset] != '\n') { 
            offset++;
            index++; 
        }
    }
    // check whehter it is the end of the string
    if (cmd[offset] == '\0') retval = 0;
    else retval = index + 1;

    cmd[index] = '\0';

    // return the length of the word trimmed
    return retval;
}

int isNotEmptyString(char *cmd) {
    while (*cmd != '\0') {
        if (*cmd++ != ' ')
            return 1;
    }
    return 0;
}

int strtok(char *cmd, char target) {
    // check for empty string
    if (!isNotEmptyString(cmd)) 
        return 0;

    int tokens = 1;
    int chars = trimWord(cmd, target);
    int cumulative = chars;
    while (chars && cmd[cumulative]) {
        chars = trimWord(&cmd[cumulative], target);
        cumulative += chars;
        tokens++;
    }
    return tokens;
}


/* source: libk.cc */
int strlen(char* str) {
    long n = 0;
    while (*str++ != 0) n++;
    return n;
}


/* http://www.geeksforgeeks.org/write-your-own-atoi/ */
int atoi(char *str) {
    int res = 0;  // Initialize result
    int sign = 1;  // Initialize sign as positive
    int i = 0;  // Initialize index of first digit
     
    // If number is negative, then update sign
    if (str[0] == '-')
    {
        sign = -1;  
        i++;  // Also update index of first digit
    }
     
    // Iterate through all digits and update the result
    for (; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';
   
    // Return result with sign
    return sign*res;
}


void dec2hex(unsigned dec, char *buf, int digits) {
    int tmp = dec;
    int length = 0;
    while (tmp / 16 > 0) {
        length++;
        tmp /= 16;
    }
    length = length > digits ? length : digits;

    buf[length + 1] = '\0';
    while (length >= 0) {
        buf[length--] = hexDigits[dec % 16];
        dec /= 16;
    }
}

/* int checkPermission(long fd, int mask) { */
/*     int uid = getuid(); */
/*     struct file_stat stats; */
/*     fstat(fd, &stats); */
/*  */
/*     // owner */
/*     if (uid == stats.uid) { */
/*         return mask & (stats.permissions >> 6) & 0x7; */
/*     } */
/*  */
/*     // we should be checking groups here, but I unfortunately */
/*     // have to skip at this moment because I don't have groups */
/*      */
/*     // other groups */
/*     return mask & stats.permissions & 0x7; */
/* } */
