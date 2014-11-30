#include "block.h"
#include "stdint.h"
#include "machine.h"
#include "debug.h"

/***************/
/* BlockDevice */
/***************/

static uint32_t min(uint32_t a, uint32_t b) {
    if (a < b) return a; else return b;
}

uint32_t BlockDevice::read(uint32_t offset, void* buf, uint32_t n) {
    uint32_t sector = offset/blockSize;
    char *data = new char[blockSize];
    readBlock(sector,data);
    uint32_t dataOffset = offset - (sector * blockSize);
    uint32_t m = min(n,blockSize-dataOffset);
    memcpy(buf,&data[dataOffset],m);
    delete[] data;
    return m;  
}

void BlockDevice::readFully(uint32_t offset, void* buf, uint32_t n) {
    uint32_t togo = n;
    char* ptr = (char*) buf;

    while (togo > 0) {
        uint32_t c = read(offset,ptr,togo);
        togo -= c;
        ptr += c;
        offset += c;
    }
}

uint32_t BlockDevice::write(uint32_t offset, void* buf, uint32_t n) {
    uint32_t sector = offset/blockSize;
    // I need to read the original content first 
    // and then overwrite the content
    char *data = new char[blockSize];

    readBlock(sector,data);

#if 0
    Debug::printf("before write:\n");
    for (uint32_t i = 0; i < blockSize; i++)
        Debug::printf("%u ", data[i]);
    Debug::printf("\n");
#endif 

    uint32_t dataOffset = offset - (sector * blockSize);
    uint32_t m = min(n, blockSize - dataOffset);
    char *buffer = (char *) buf;
    // overwrite the content of the block
    memcpy(&data[dataOffset], buffer, m);
    // write the block back to the disk
    writeBlock(sector, data);

#if 0
    // verify
    readBlock(sector,data);
    for (uint32_t i = 0; i < blockSize; i++)
        Debug::printf("%u ", data[i]);
    Debug::printf("\n");
#endif 

    delete[] data;
    return m;
}

void BlockDevice::writeFully(uint32_t offset, void* buf, uint32_t n) {
    uint32_t togo = n;
    char* ptr = (char*) buf;

    while (togo > 0) {
        uint32_t c = write(offset,ptr,togo);
        togo -= c;
        ptr += c;
        offset += c;
    }
}
