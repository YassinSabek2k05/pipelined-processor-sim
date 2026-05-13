#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>



void init_processor(void);
void step_cycle(void);
void run_simulation(void);
void write_register(uint8_t reg_idx, uint32_t value);

void fetch_stage(void);
void decode_stage(void);
void execute_stage(void);
void memory_stage(void);
void writeback_stage(void);

#endif // PIPELINE_H
