#include "../include/memory.h"
#include <stdint.h>
#include <string.h>

uint32_t memory[MEMORY_SIZE];

void init_memory(void) {
    memset(memory, 0, sizeof(memory));
}

uint32_t read_memory(uint32_t address) {
    if (address >= MEMORY_SIZE) {
        return 0;
    }
    return memory[address];
}

void write_memory(uint32_t address, uint32_t value) {
    if (address >= MEMORY_SIZE) {
        return;
    }
    memory[address] = value;
}
