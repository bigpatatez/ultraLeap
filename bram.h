#ifndef BRAM_H
#define BRAM_H
#include <stdbool.h>
#include <stdint.h>

typedef struct{
	int fd;
    	uint32_t* bram;
} BRAMReader;
int initBRAMReader(BRAMReader* reader);
void cleanupBRAMReader(BRAMReader* reader);
int readBRAMData(BRAMReader* reader, size_t offset, uint32_t* data);
int writeBRAMData(BRAMReader* reader, size_t offset, uint32_t data);
bool checkBits(uint32_t value, uint32_t bitmask);
int modifyBRAMBits(BRAMReader* reader, size_t offset, uint32_t mask, uint32_t bitValues);
uint32_t extract_bits(uint32_t value, int start, int length);
#endif 
