//
// Created by pc on 5/13/2026.
//

#include <stdbool.h>
#include <stdint.h>


#ifndef PIPELINED_PROCESSOR_SIM_STRUCTS_H
#define PIPELINED_PROCESSOR_SIM_STRUCTS_H
enum ACCESS {
    READ = 0,
    WRITE = 1
};
enum OPCODE {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    MOVI = 3,
    JEQ = 4,
    AND = 5,
    ORI = 6,
    JMP = 7,
    LSL = 8,
    LSR = 9,
    MOVR = 10,
    MOVM = 11,
  };
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
    int cycles_in_stage;    // 1 or 2

    // decoded fields
    enum OPCODE opcode;
    uint32_t r1, r2, r3;
    uint32_t shamt;
    uint32_t address;
    int32_t  imm;

    // register values
    int32_t val1;
    int32_t val2;

    // control signals
} ID_EX_Reg;

typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    int cycles_in_stage;    // 1 or 2

    int32_t  alu_result;
    int32_t  store_data;    // for MOVM
    uint32_t dest;

    enum ACCESS mem_access;
    bool reg_write;

} EX_MEM_Reg;

typedef struct {
    uint32_t instruction;
    uint32_t pc;
    bool valid;

    int32_t  alu_result;
    int32_t  memory_data;
    uint32_t dest;

} MEM_WB_Reg;


/**
 * PROCESSOR STATE
 */
typedef struct {
    // Architectural State
    uint32_t pc;
    int32_t  R[33];

    // Simulation Control
    uint32_t cycles;
    bool running;

    IF_ID_Reg  if_id;
    ID_EX_Reg  id_ex;
    EX_MEM_Reg ex_mem;
    MEM_WB_Reg mem_wb;

    // Hazard State
    bool stall;
    bool flush;
} ProcessorState;

#define MEMORY_SIZE 2048
#define INSTRUCTION_SEGMENT_END 1024
#define DATA_SEGMENT_START 1024

#endif //PIPELINED_PROCESSOR_SIM_STRUCTS_H