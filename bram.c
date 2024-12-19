
#define BRAM_BASE_ADDR 0x90000000
#define BRAM_SIZE 0x2000 // Adjust as needed
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>

typedef struct {
    int fd;
    uint32_t* bram;
} BRAMReader;



// Function to initialize BRAMReader
int initBRAMReader(BRAMReader* reader) {
    reader->fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (reader->fd == -1) {
        perror("open");
        return -1;
    }

    reader->bram = (uint32_t*)mmap(NULL, BRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, reader->fd, BRAM_BASE_ADDR);
    if (reader->bram == MAP_FAILED) {
        perror("mmap");
        close(reader->fd);
        return -1;
    }

    return 0;
}

// Function to clean up BRAMReader
void cleanupBRAMReader(BRAMReader* reader) {
    if (reader->bram != MAP_FAILED) {
        munmap(reader->bram, BRAM_SIZE);
    }
    if (reader->fd != -1) {
        close(reader->fd);
    }
}

// Function to read data from BRAM
int readBRAMData(BRAMReader* reader, size_t offset, uint32_t* data) {
    if (offset >= (BRAM_SIZE / sizeof(uint32_t))) {
        fprintf(stderr, "Offset out of bounds\n");
        return -1;
    }
    *data = reader->bram[offset];
    return 0;
}

// Function to write data to BRAM
int writeBRAMData(BRAMReader* reader, size_t offset, uint32_t data) {
    if (offset >= (BRAM_SIZE / sizeof(uint32_t))) {
        fprintf(stderr, "Offset out of bounds\n");
        return -1;
    }
    reader->bram[offset] = data;
    return 0;
}

int modifyBRAMBits(BRAMReader* reader, size_t offset, uint32_t mask, uint32_t bitValues){
    if (offset >= (BRAM_SIZE / sizeof(uint32_t))) {
        fprintf(stderr, "Offset out of bounds\n");
        return -1;
    }
    bitValues &= mask;
    reader->bram[offset] = (reader->bram[offset] & ~mask) | bitValues;
    return 0;
}
// Function to check if specific bits are set
bool checkBits(uint32_t value, uint32_t bitmask) {
    return (value & bitmask) == bitmask;
}

// Function to extract a range of bits
uint32_t extract_bits(uint32_t value, int start, int length) {
    if (start < 0 || start >= 32 || length <= 0 || start + length > 32) {
        fprintf(stderr, "Invalid bit range\n");
        return 0;
    }

    // Create a mask for the desired number of bits
    uint32_t mask = (1U << length) - 1;

    // Shift the value right to align the range, then apply the mask
    return (value >> start) & mask;
}

