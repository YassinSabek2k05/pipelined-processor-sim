#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MEMORY_SIZE 2048
#define INSTRUCTION_SEGMENT_END 1024
#define DATA_SEGMENT_START 1024

extern uint32_t memory[MEMORY_SIZE];

void init_memory();
uint32_t read_memory(uint32_t address);
void write_memory(uint32_t address, uint32_t value);

#endif