#ifndef BRAM_H
#define BRAM_H

typedef struct{
	int fd;
    	uint32_t* bram;
} BRAMReader;
int initBRAMReader(BRAMReader* reader);
void cleanupBRAMReader(BRAMReader* reader);
int readBRAMData(BRAMReader* reader, size_t offset, uint32_t* data);
int writeBRAMData(BRAMReader* reader, size_t offset, uint32_t data);
bool checkBits(uint32_t value, uint32_t bitmask);
#endif 
