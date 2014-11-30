/**
 * Source:
 * http://svn.ghostscript.com/jbig2dec/trunk/sha1.h
 * http://svn.ghostscript.com/jbig2dec/trunk/sha1.c
 * */

/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

#ifndef __SHA1_H
#define __SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "libc.h"

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} SHA1_CTX;

#define SHA1_DIGEST_SIZE 20

void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context,  uint8_t* data,  size_t len);
void SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE]);

char* SHA1_Generate(char *str);
int SHA1_Verify(char *hashed, char *str);

#ifdef __cplusplus
}
#endif

#endif /* __SHA1_H */
