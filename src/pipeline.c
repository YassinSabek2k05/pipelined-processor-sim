#include "../include/pipeline.h"
#include "../include/memory.h"
#include "../include/loader.h"
#include <stdio.h>
#include <string.h>
#include "../include/structs.h"

ProcessorState cpu;

void init_processor(void) {
    // Reset all architectural and pipeline state
    memset(&cpu, 0, sizeof(cpu));
    
    // Architectural Reset: PC starts at 0, R0 is hard-wired to 0
    cpu.pc = 0;
    cpu.R[0] = 0; 
    
    cpu.cycles = 0;
    cpu.running = true;
}

void fetch_stage(void) {
    // Structural Hazard: Only fetch on ODD cycles in Package 2
    if (cpu.cycles % 2 != 1) {
        printf("Fetch Stage: Idle (Waiting for even cycle to finish MEM)\n");
        return;
    }

    if (cpu.pc < get_instruction_count()) {
        // 1. ARCHITECTURAL ACTION: Use PC to fetch
        cpu.if_id.instruction = read_memory(cpu.pc);
        cpu.if_id.pc = cpu.pc;
        cpu.if_id.valid = true;
        printf("Fetch Stage: Fetching instruction 0x%08X from address %u\n",
               cpu.if_id.instruction, cpu.pc);

        // 2. ARCHITECTURAL ACTION: Increment PC
        // "As each instruction gets fetched, the program counter is incremented"
        cpu.pc++;
    } else {
        cpu.if_id.valid = false;
        printf("Fetch Stage: No more instructions to fetch (PC=%u)\n", cpu.pc);
    }
}
int32_t sign_extend_18(uint32_t imm);

int32_t sign_extend_18(uint32_t imm) {
    if (imm & 0x20000) { // Check if 18th bit is 1
        return (int32_t)(imm | 0xFFFC0000); // Sign extend
    }
    return (int32_t)imm;
}

void decode_stage(void) {

    if (!cpu.if_id.valid) {
        printf("Decode Stage: Idle\n");
        return;
    }
    cpu.id_ex.cycles_in_stage++;

    if (cpu.id_ex.cycles_in_stage==1) {
        cpu.id_ex.valid=false;
        cpu.id_ex.pc = cpu.if_id.pc;
        cpu.id_ex.instruction = cpu.if_id.instruction;
        cpu.id_ex.opcode = (enum OPCODE)(cpu.id_ex.instruction >> 28);
        cpu.id_ex.r1 = cpu.id_ex.instruction >> 23 & 0x1F;
        cpu.id_ex.r2 = cpu.id_ex.instruction >> 18 & 0x1F;
        cpu.id_ex.r3 = cpu.id_ex.instruction >> 13 & 0x1F;
        cpu.id_ex.address = cpu.id_ex.address & 0x0FFFFFFF;
        cpu.id_ex.shamt = cpu.id_ex.instruction & 0x1FFF;
        cpu.id_ex.imm = sign_extend_18(cpu.id_ex.instruction & 0x3FFFF);
        cpu.id_ex.val1 = cpu.R[cpu.id_ex.r1];
        cpu.id_ex.val2 = cpu.R[cpu.id_ex.r2];

        printf("Decode Stage: Decoding instruction 0x%08X (Cycle %d/2)---", cpu.id_ex.instruction, cpu.id_ex.cycles_in_stage);
        printf("Opcode: %d---", cpu.id_ex.opcode);
        printf("R1: %d, Val1: %d ---", cpu.id_ex.r1, cpu.id_ex.val1);
        printf("R2: %d, Val2: %d ---", cpu.id_ex.r2, cpu.id_ex.val2);
        printf("R3: %d ---", cpu.id_ex.r3);
        printf("Imm: %d ---", cpu.id_ex.imm);
        printf("Shamt: %d ---", cpu.id_ex.shamt);
        printf("Address: %d\n", cpu.id_ex.address);
    }
    else if (cpu.id_ex.cycles_in_stage== 2) {
        cpu.id_ex.valid=true;
        cpu.if_id.valid=false;
        printf("Decode Stage: Decoding instruction 0x%08X (Cycle %d/2) - Opcode: %d\n",
               cpu.id_ex.instruction, cpu.id_ex.cycles_in_stage, cpu.id_ex.opcode);
        cpu.id_ex.cycles_in_stage=0;
    }

}

void execute_stage(void) {
    if (!cpu.id_ex.valid) return;

    cpu.ex_mem.cycles_in_stage++;

    if (cpu.ex_mem.cycles_in_stage == 1) {
        switch (cpu.id_ex.opcode) {
            case ADD:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 + cpu.id_ex.val2;
                printf("----------------\nalu=%d\n----------------\n",cpu.ex_mem.alu_result);
                break;
            case SUB:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 - cpu.id_ex.val2;
                break;
            case MUL:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 * cpu.id_ex.val2;
                break;
            case MOVI:
                cpu.ex_mem.alu_result=cpu.id_ex.imm;
                break;
            case AND:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 & cpu.id_ex.val2;
                break;
            case ORI:
                cpu.ex_mem.alu_result = cpu.id_ex.val2 | cpu.id_ex.imm;
                break;
            case LSL:
                cpu.ex_mem.alu_result = cpu.id_ex.val2 << cpu.id_ex.shamt ;
                break;
            case LSR:
                cpu.ex_mem.alu_result = cpu.id_ex.val2 >> cpu.id_ex.shamt;
                break;
            case MOVR:
                cpu.ex_mem.mem_access = WRITE;
                cpu.ex_mem.alu_result = cpu.id_ex.val2;
                break;
            case MOVM:
                cpu.ex_mem.mem_access = READ;
                cpu.ex_mem.alu_result = cpu.id_ex.val2 + cpu.id_ex.imm;
                break;
            default: printf("alu skipped");
        }
        // compute ALU result
        // store in ex_mem
    }

    if (cpu.ex_mem.cycles_in_stage == 2) {
        // evaluate branch/jump
        // if taken: flush, update PC
        // move to MEM
        cpu.ex_mem.cycles_in_stage = 0;
        cpu.id_ex.valid=false;
        cpu.ex_mem.valid=true;
    }

}

void memory_stage(void) {
    // Structural Hazard: Only MEM access on EVEN cycles
    if (cpu.cycles % 2 != 0 || !cpu.ex_mem.valid) {
        printf("Memory Stage: Idle\n");
        return;
    }

    cpu.ex_mem.valid=false;
    cpu.mem_wb.valid=true;
    printf("Memory Stage: Accessing memory for instruction 0x%08X\n", cpu.ex_mem.instruction);
}

void writeback_stage(void) {
    if (cpu.cycles % 2 == 0 || !cpu.mem_wb.valid) {
        printf("Write Back Stage: Idle\n");
        return;
    }
    cpu.mem_wb.valid=false;
    printf("Write Back Stage: Writing back result for instruction 0x%08X\n", cpu.mem_wb.instruction);
}

void step_cycle(void) {
    cpu.cycles++;
    printf("--- Clock Cycle %u ---\n", cpu.cycles);

    // RUN STAGES
    writeback_stage();
    memory_stage();
    execute_stage();
    decode_stage();
    fetch_stage();

    if (cpu.pc >= get_instruction_count() && 
        !cpu.if_id.valid && !cpu.id_ex.valid &&
        !cpu.ex_mem.valid && !cpu.mem_wb.valid) {
        cpu.running = false;
    }
}

/**
 * ARCHITECTURAL RULE: R0 is hard-wired to zero.
 * This helper ensures any write to R0 is ignored.
 */
void write_register(uint8_t reg_idx, uint32_t value) {
    if (reg_idx == 0) {
        // "Hard-wired value 0 (cannot be overwritten by any instruction)"
        return; 
    }
    if (reg_idx < NUM_GPR) {
        cpu.R[reg_idx] = value;
    }
}

void print_final_state(void) {
    printf("\n=== Final Architectural Register State (The 33 Registers) ===\n");
    printf("PC: %u\n", cpu.pc);
    for (int i = 0; i < NUM_GPR; i++) {
        printf("R%d: %u (0x%08X)\n", i, cpu.R[i], cpu.R[i]);
    }
    
    printf("\n=== Memory Dump ===\n");
    // ... (Same memory print logic as before)
    for (int i = 0; i < 2048; i++) {
        uint32_t val = read_memory(i);
        if (val != 0) printf("Addr %d: 0x%08X\n", i, val);
    }
}

void run_simulation(void) {
    printf("Starting simulation (Package 2)...\n");
    while (cpu.running) {
        step_cycle();
    }
    printf("Simulation finished after %u cycles.\n", cpu.cycles);
    print_final_state();
}
