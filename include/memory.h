#ifndef MEMORY_H
#define MEMORY_H

#include "structs.h"
#include <stdint.h>



extern uint32_t memory[MEMORY_SIZE];

void init_memory();
uint32_t read_memory(uint32_t address);
void write_memory(uint32_t address, uint32_t value);

#endif