#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * 1. ARCHITECTURAL REGISTERS (33 Registers Total)
 * These are the registers defined by the ISA for Package 2.
 */
#define NUM_GPR 32  // R0 to R31 (32 registers)
// Total = 32 GPRs + 1 PC = 33 Registers.

/**
 * 2. PIPELINE (MICROARCHITECTURAL) REGISTERS
 * These are internal hardware latches between stages. 
 * They are NOT part of the 33 architectural registers.
 */
typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
} IF_ID_Reg;

typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    uint32_t opcode, r1, r2, r3, shamt, address;
    int32_t imm;
} ID_EX_Reg;

typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    uint32_t alu_result;
} EX_MEM_Reg;

typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    uint32_t alu_result;
    uint32_t memory_data;
    uint32_t reg_dest;
} MEM_WB_Reg;

/**
 * PROCESSOR STATE
 */
typedef struct {
    // --- Architectural State (The 33 Registers) ---
    uint32_t pc;              // Register #33: Program Counter
    uint32_t R[NUM_GPR];      // Registers #1-32: R0 to R31 (R0 is hard-wired 0)

    // --- Simulation State ---
    uint32_t cycles;
    bool running;

    // --- Pipeline Latches (Internal Hardware) ---
    IF_ID_Reg if_id;
    ID_EX_Reg id_ex;
    EX_MEM_Reg ex_mem;
    MEM_WB_Reg mem_wb;

    // Stage occupancy tracking
    struct { uint32_t instruction; bool valid; } id_stage, ex_stage, mem_stage, wb_stage;
} ProcessorState;

extern ProcessorState cpu;

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
