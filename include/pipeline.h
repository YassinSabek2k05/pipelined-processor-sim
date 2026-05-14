#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include "structs.h"

// ── Architectural state (defined in pipeline.c) ──────────────────────────────
extern ProcessorState cpu;

// ── Stage functions ───────────────────────────────────────────────────────────
void init_processor(void);
void step_cycle(void);
void run_simulation(void);
void write_register(uint8_t reg_idx, uint32_t value);

void fetch_stage(void);
void decode_stage(void);
void execute_stage(void);
void memory_stage(void);
void writeback_stage(void);

// ── GUI pipeline-history types ────────────────────────────────────────────────
#define MAX_GUI_HISTORY 200

typedef struct {
    // stage_pc[0]=IF  [1]=ID  [2]=EX  [3]=MEM  [4]=WB
    // -1 means idle for that stage this cycle
    int stage_pc[5];
} PipelineSnapshot;

extern PipelineSnapshot gui_snapshots[MAX_GUI_HISTORY];
extern int              gui_snapshot_count;

// Called from within each stage to record which instruction (by PC addr) is active
void gui_record_stage(int stage, int pc_addr);

#endif // PIPELINE_H