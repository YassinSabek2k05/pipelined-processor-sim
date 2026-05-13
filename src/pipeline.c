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
bool detect_load_use_hazard(void) {
    // is the instruction in EX/MEM a load?
    if (cpu.ex_mem.mem_access != READ) return false;

    // does the instruction about to enter EX need that register?
    if (cpu.ex_mem.dest == cpu.id_ex.r1 ||
        cpu.ex_mem.dest == cpu.id_ex.r2) {
        return true;
    }
    return false;
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

        // Decode register fields FIRST, then use them for forwarding
        cpu.id_ex.r1 = (cpu.id_ex.instruction >> 23) & 0x1F;
        cpu.id_ex.r2 = (cpu.id_ex.instruction >> 18) & 0x1F;
        cpu.id_ex.r3 = (cpu.id_ex.instruction >> 13) & 0x1F;
        cpu.id_ex.address = cpu.id_ex.instruction & 0x0FFFFFFF;
        cpu.id_ex.shamt = cpu.id_ex.instruction & 0x1FFF;
        cpu.id_ex.imm = sign_extend_18(cpu.id_ex.instruction & 0x3FFFF);


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
    if (!cpu.id_ex.valid && cpu.ex_mem.cycles_in_stage != 1) return;

    cpu.ex_mem.cycles_in_stage++;

    if (cpu.ex_mem.cycles_in_stage == 1) {
        // Latch new instruction into ex_mem and compute ALU result
        // operand 1 (r1)
        if (cpu.ex_mem.valid && cpu.ex_mem.reg_write && cpu.id_ex.r1 == cpu.ex_mem.dest) {
            cpu.id_ex.val1 = cpu.ex_mem.alu_result;
        } else if (cpu.mem_wb.valid && cpu.mem_wb.reg_write && cpu.id_ex.r1 == cpu.mem_wb.dest) {
            cpu.id_ex.val1 = cpu.mem_wb.alu_result;
        } else {
            cpu.id_ex.val1 = cpu.R[cpu.id_ex.r1];
        }

        // operand 2 (r2)
        if (cpu.ex_mem.valid && cpu.ex_mem.reg_write && cpu.id_ex.r2 == cpu.ex_mem.dest) {
            cpu.id_ex.val2 = cpu.ex_mem.alu_result;
        } else if (cpu.mem_wb.valid && cpu.mem_wb.reg_write && cpu.id_ex.r2 == cpu.mem_wb.dest) {
            cpu.id_ex.val2 = cpu.mem_wb.alu_result;
        } else {
            cpu.id_ex.val2 = cpu.R[cpu.id_ex.r2];
        }

        cpu.ex_mem.instruction   = cpu.id_ex.instruction;
        cpu.ex_mem.pc            = cpu.id_ex.pc;
        cpu.ex_mem.valid         = false;
        cpu.ex_mem.reg_write     = false;
        cpu.ex_mem.mem_access    = NONE;
        cpu.ex_mem.dest          = 0;
        cpu.ex_mem.alu_result    = 0;
        cpu.ex_mem.store_data    = 0;
        cpu.ex_mem.branch_taken  = false;
        cpu.ex_mem.branch_target = 0;

        printf("Execute: cycle 1/2 op: %d\n", cpu.id_ex.opcode);
        switch (cpu.id_ex.opcode) {
            case ADD:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 + cpu.id_ex.val2;
                cpu.ex_mem.dest       = cpu.id_ex.r3;
                cpu.ex_mem.reg_write  = true;
                printf("----------------\nalu=%d\n----------------\n", cpu.ex_mem.alu_result);
                break;
            case SUB:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 - cpu.id_ex.val2;
                cpu.ex_mem.dest       = cpu.id_ex.r3;
                cpu.ex_mem.reg_write  = true;
                break;
            case MUL:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 * cpu.id_ex.val2;
                cpu.ex_mem.dest       = cpu.id_ex.r3;
                cpu.ex_mem.reg_write  = true;
                break;
            case MOVI:
                cpu.ex_mem.alu_result = cpu.id_ex.imm;
                cpu.ex_mem.dest       = cpu.id_ex.r1;
                cpu.ex_mem.reg_write  = true;
                printf("dest %d----------alu %d\n ", cpu.ex_mem.dest,cpu.ex_mem.alu_result);
                break;
            case AND:
                cpu.ex_mem.alu_result = cpu.id_ex.val1 & cpu.id_ex.val2;
                cpu.ex_mem.dest       = cpu.id_ex.r3;
                cpu.ex_mem.reg_write  = true;
                break;
            case ORI:
                cpu.ex_mem.alu_result = cpu.id_ex.val2 | cpu.id_ex.imm;
                cpu.ex_mem.dest       = cpu.id_ex.r1;
                cpu.ex_mem.reg_write  = true;
                break;
            case LSL:
                cpu.ex_mem.alu_result = cpu.id_ex.val2 << cpu.id_ex.shamt;
                cpu.ex_mem.dest       = cpu.id_ex.r1;
                cpu.ex_mem.reg_write  = true;
                break;
            case LSR:
                cpu.ex_mem.alu_result = (uint32_t)cpu.id_ex.val2 >> cpu.id_ex.shamt;
                cpu.ex_mem.dest       = cpu.id_ex.r1;
                cpu.ex_mem.reg_write  = true;
                break;
            case MOVR:
                cpu.ex_mem.mem_access = READ;
                cpu.ex_mem.alu_result = cpu.id_ex.val2+cpu.id_ex.imm;
                cpu.ex_mem.dest = cpu.id_ex.r1;
                cpu.ex_mem.reg_write  = true;
                break;
            case MOVM:
                cpu.ex_mem.mem_access = WRITE;
                cpu.ex_mem.alu_result = cpu.id_ex.val2+cpu.id_ex.imm;
                cpu.ex_mem.store_data = cpu.id_ex.val1;
                cpu.ex_mem.reg_write  = false;
                break;
            case JMP:
                cpu.ex_mem.branch_taken  = true;
                cpu.ex_mem.branch_target = cpu.id_ex.address;
                cpu.ex_mem.reg_write     = false;
                break;
            case JEQ:
                cpu.ex_mem.branch_taken = (cpu.id_ex.val1==cpu.id_ex.val2);
                cpu.ex_mem.branch_target = (cpu.id_ex.pc& 0xF0000000) + (cpu.id_ex.address);
                cpu.ex_mem.reg_write    = false;
                break;
            default:
                printf("invalid error\n");
                break;
        }
    }

    if (cpu.ex_mem.cycles_in_stage == 2) {
        printf("Execute: cycle 2/2\n");
        printf("alu in cycle 2 %d\n", cpu.ex_mem.alu_result);
        cpu.ex_mem.cycles_in_stage = 0;
        cpu.id_ex.valid  = false;
        cpu.ex_mem.valid = true;

        if (cpu.ex_mem.branch_taken) {
            cpu.pc = cpu.ex_mem.branch_target;
            //flushing
            cpu.if_id.valid          = false;
            cpu.id_ex.valid          = false;
            cpu.id_ex.cycles_in_stage = 0;
            cpu.ex_mem.branch_taken  = false;
            printf("Branch taken: flushing pipeline, jumping to PC=%u\n", cpu.pc);
        }
    }
}

void memory_stage(void) {
    // Structural Hazard: Only MEM access on EVEN cycles
    if (cpu.cycles % 2 != 0 || !cpu.ex_mem.valid) {
        printf("Memory Stage: Idle\n");
        return;
    }
    cpu.mem_wb.instruction = cpu.ex_mem.instruction;
    cpu.mem_wb.pc          = cpu.ex_mem.pc;
    cpu.mem_wb.alu_result  = cpu.ex_mem.alu_result;
    cpu.mem_wb.dest        = cpu.ex_mem.dest;
    cpu.mem_wb.reg_write   = cpu.ex_mem.reg_write;
    if (cpu.ex_mem.mem_access==WRITE) {
        write_memory(cpu.ex_mem.alu_result, cpu.ex_mem.store_data);
    }
    else if (cpu.ex_mem.mem_access==READ) {
        cpu.mem_wb.alu_result = read_memory(cpu.ex_mem.alu_result);
    }
    printf("dest %d in mem stage\n", cpu.ex_mem.dest);
    cpu.ex_mem.valid = false;
    cpu.mem_wb.valid = true;
    printf("Memory Stage: Accessing memory for instruction 0x%08X\n", cpu.ex_mem.instruction);
}

void writeback_stage(void) {
    if (cpu.cycles % 2 == 0 || !cpu.mem_wb.valid) {
        printf("Write Back Stage: Idle\n");
        return;
    }
    if (cpu.mem_wb.reg_write) {
        printf("writing %d to register %d\n", cpu.mem_wb.alu_result, cpu.mem_wb.dest);
        write_register(cpu.mem_wb.dest, cpu.mem_wb.alu_result);
    }
    cpu.mem_wb.valid = false;
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