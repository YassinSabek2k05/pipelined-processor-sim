#include "../include/pipeline.h"
#include "../include/memory.h"
#include "../include/loader.h"
#include <stdio.h>
#include <string.h>
#include "../include/parser.h"


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

// helpers to extract fields from the instruction code
static uint32_t get_opcode(uint32_t inst) {
    return (uint32_t) (inst >> 28) & 0xF;
}

static uint32_t get_r1(uint32_t inst) {
    return (inst >> 23) & 0x1F;
}

static uint32_t get_r2(uint32_t inst) {
    return (inst >> 18) & 0x1F;
}

static uint32_t get_r3(uint32_t inst) {
    return (inst >> 13) & 0x1F;
}

static uint32_t get_shamt(uint32_t inst) {
    return inst & 0x1FFF;
}

static uint32_t get_address(uint32_t inst) {
    return inst & 0x0FFFFFFF;
}

static int32_t sign_extend_18(uint32_t imm) { // extend immediate to 32 bits
    if (imm & (1 << 17)) {
        imm |= 0xFFFC0000;
    }

    return (int32_t) imm;
}

void decode_stage(void) { // 2 cycles
    if (!cpu.id_stage.valid) {
        printf("Decode Stage: Idle\n");
        return;
    }

    int stage_cycle = (cpu.cycles % 2 == 0) ? 1 : 2;

    if (stage_cycle == 1){ // cycle 1
        printf("Decode Stage: Decoding instruction 0x%08X (Cycle 1/2)\n", cpu.id_stage.instruction);

        uint32_t inst = cpu.id_stage.instruction;

        cpu.id_ex.valid = true;

        cpu.id_ex.instruction = inst;
        cpu.id_ex.pc = cpu.if_id.pc;

        cpu.id_ex.opcode = get_opcode(inst);

        cpu.id_ex.r1 = get_r1(inst);
        cpu.id_ex.r2 = get_r2(inst);
        cpu.id_ex.r3 = get_r3(inst);

        cpu.id_ex.shamt = get_shamt(inst);

        cpu.id_ex.address = get_address(inst);

        cpu.id_ex.imm = sign_extend_18(inst & 0x3FFFF);

        cpu.id_ex.val1 = cpu.R[cpu.id_ex.r1];
        cpu.id_ex.val2 = cpu.R[cpu.id_ex.r2];

        switch (cpu.id_ex.opcode) { // adjusting control signals

        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_AND:

            cpu.id_ex.dest = cpu.id_ex.r3;

            cpu.id_ex.control.regWrite = true;
            cpu.id_ex.control.aluSrc = false; // R-format
            break;

        case OPCODE_ORI:
        case OPCODE_MOVI:

            cpu.id_ex.dest = cpu.id_ex.r1;

            cpu.id_ex.control.regWrite = true;
            cpu.id_ex.control.aluSrc = true; // I-format
            break;

        case OPCODE_LSR:
        case OPCODE_LSL:

            cpu.id_ex.dest = cpu.id_ex.r1;

            cpu.id_ex.control.regWrite = true;

            break;

        case OPCODE_MOVR:

            cpu.id_ex.dest = cpu.id_ex.r1;

            cpu.id_ex.control.memRead = true;
            cpu.id_ex.control.memToReg = true;
            cpu.id_ex.control.regWrite = true;
            break;

        case OPCODE_MOVM:

            cpu.id_ex.control.memWrite = true;
            break;

        case OPCODE_JEQ:

            cpu.id_ex.control.branch = true;
            break;

        case OPCODE_JMP:

            cpu.id_ex.control.jump = true;
            break;
        }

        // printf("================ DECODE STAGE ================\n");
        // printf("PC                : %u\n", cpu.id_ex.pc);
        // printf("Instruction        : 0x%08X\n", cpu.id_ex.instruction);
        // printf("Opcode             : %u\n", cpu.id_ex.opcode);
        //
        // printf("r1                 : R%u\n", cpu.id_ex.r1);
        // printf("r2                 : R%u\n", cpu.id_ex.r2);
        // printf("r3                 : R%u\n", cpu.id_ex.r3);
        //
        // printf("val1               : %d\n", cpu.id_ex.val1);
        // printf("val2               : %d\n", cpu.id_ex.val2);
        //
        // printf("Immediate          : %d\n", cpu.id_ex.imm);
        // printf("Shamt              : %u\n", cpu.id_ex.shamt);
        // printf("Address            : %u\n", cpu.id_ex.address);
        //
        // printf("Destination        : R%u\n", cpu.id_ex.dest);
        //
        // printf("Control Signals:\n");
        // printf("  regWrite         : %d\n", cpu.id_ex.control.regWrite);
        // printf("  memRead          : %d\n", cpu.id_ex.control.memRead);
        // printf("  memWrite         : %d\n", cpu.id_ex.control.memWrite);
        // printf("  memToReg         : %d\n", cpu.id_ex.control.memToReg);
        // printf("  aluSrc           : %d\n", cpu.id_ex.control.aluSrc);
        // printf("  branch           : %d\n", cpu.id_ex.control.branch);
        // printf("  jump             : %d\n", cpu.id_ex.control.jump);
        //
        // printf("================================================\n");
    }else{ // cycle 2
        printf("Decode Stage: Waiting... (Cycle 2/2)\n");
    }
}

void execute_stage(void) {
    if (!cpu.ex_stage.valid) {
        printf("Execute Stage: Idle\n");
        return;
    }
    int stage_cycle = (cpu.cycles % 2 == 0) ? 1 : 2;

    if (stage_cycle == 1){ // cycle 1
        cpu.ex_mem.valid = true;
        cpu.ex_mem.store_data = cpu.id_ex.val1;
        cpu.ex_mem.instruction = cpu.id_ex.instruction;
        cpu.ex_mem.pc = cpu.id_ex.pc;

        cpu.ex_mem.dest = cpu.id_ex.dest;

        cpu.ex_mem.control = cpu.id_ex.control;

        int32_t operand2 = cpu.id_ex.control.aluSrc
                            ? cpu.id_ex.imm
                            : cpu.id_ex.val2;

        switch (cpu.id_ex.opcode) {

        case OPCODE_ADD:
            cpu.ex_mem.alu_result = cpu.id_ex.val1 + operand2;
            break;

        case OPCODE_SUB:
            cpu.ex_mem.alu_result = cpu.id_ex.val1 - operand2;
            break;

        case OPCODE_MUL:
            cpu.ex_mem.alu_result = cpu.id_ex.val1 * operand2;
            break;

        case OPCODE_AND:
            cpu.ex_mem.alu_result = cpu.id_ex.val1 & operand2;
            break;

        case OPCODE_ORI:
            cpu.ex_mem.alu_result = cpu.id_ex.val1 | cpu.id_ex.imm;
            break;

        case OPCODE_MOVI:
            cpu.ex_mem.alu_result = cpu.id_ex.imm;
            break;

        case OPCODE_LSL:
            cpu.ex_mem.alu_result = cpu.id_ex.val2 << cpu.id_ex.shamt;
            break;

        case OPCODE_LSR:
            cpu.ex_mem.alu_result = cpu.id_ex.val2 >> cpu.id_ex.shamt;
            break;

        case OPCODE_MOVR:
        case OPCODE_MOVM:
            cpu.ex_mem.alu_result = cpu.id_ex.val2 + cpu.id_ex.imm;
            break;
        }

        // printf("================ EXECUTE STAGE ================\n");
        // printf("PC                : %u\n", cpu.ex_mem.pc);
        // printf("Instruction        : 0x%08X\n", cpu.ex_mem.instruction);
        // printf("Opcode             : %u\n", cpu.id_ex.opcode);
        //
        // printf("Operand 1          : %d\n", cpu.id_ex.val1);
        // printf("Operand 2          : %d\n", operand2);
        //
        // printf("ALU Result         : %u\n", cpu.ex_mem.alu_result);
        //
        // printf("Destination        : R%u\n", cpu.ex_mem.dest);
        //
        // printf("Control Signals:\n");
        // printf("  regWrite         : %d\n", cpu.ex_mem.control.regWrite);
        // printf("  memRead          : %d\n", cpu.ex_mem.control.memRead);
        // printf("  memWrite         : %d\n", cpu.ex_mem.control.memWrite);
        // printf("  memToReg         : %d\n", cpu.ex_mem.control.memToReg);
        //
        // printf("================================================\n");
    }else{ // cycle 2
        printf("Execute Stage: Evaluating Branches (Cycle 2/2)\n");

        if (cpu.id_ex.opcode == OPCODE_JEQ) {

            if (cpu.id_ex.val1 == cpu.id_ex.val2) {

                cpu.pc = cpu.id_ex.pc + 1 + cpu.id_ex.imm;

                cpu.if_id.valid = false;
                cpu.id_stage.valid = false;

                printf("Branch Taken -> PC=%u\n", cpu.pc);
            }
        }

        if (cpu.id_ex.opcode == OPCODE_JMP) {

            cpu.pc =
                (cpu.id_ex.pc & 0xF0000000)
                | cpu.id_ex.address;

            cpu.if_id.valid = false;
            cpu.id_stage.valid = false;

            printf("Jump Taken -> PC=%u\n", cpu.pc);
        }
    }
}

void memory_stage(void) { // 1 cycle
    // Structural Hazard: Only MEM access on EVEN cycles
    if (cpu.cycles % 2 != 0 || !cpu.mem_stage.valid) {
        printf("Memory Stage: Idle\n");
        return;
    }
    printf("Memory Stage: Accessing memory for instruction 0x%08X\n", cpu.mem_stage.instruction);

    cpu.mem_wb.valid = true;

    cpu.mem_wb.instruction = cpu.ex_mem.instruction;
    cpu.mem_wb.pc = cpu.ex_mem.pc;

    cpu.mem_wb.dest = cpu.ex_mem.dest;

    cpu.mem_wb.alu_result = cpu.ex_mem.alu_result;

    cpu.mem_wb.control = cpu.ex_mem.control;

    if (cpu.ex_mem.control.memRead) {

        cpu.mem_wb.memory_data =
            read_memory(cpu.ex_mem.alu_result);

        printf("Memory Read [%u] = %u\n",
               cpu.ex_mem.alu_result,
               cpu.mem_wb.memory_data);
    }

    if (cpu.ex_mem.control.memWrite){
        write_memory(cpu.ex_mem.alu_result,
                         cpu.ex_mem.store_data); // <--- Correctly read from EX/MEM

        printf("Memory Write [%u] = %u\n",
               cpu.ex_mem.alu_result,
               cpu.ex_mem.store_data);
    }

}

void writeback_stage(void) { //  1 cycle
    if (cpu.cycles % 2 == 0 || !cpu.wb_stage.valid) {
        printf("Write Back Stage: Idle\n");
        return;
    }
    printf("Write Back Stage: Writing back result for instruction 0x%08X\n", cpu.wb_stage.instruction);

    if (cpu.mem_wb.control.regWrite) {

        uint32_t value =
            cpu.mem_wb.control.memToReg
            ? cpu.mem_wb.memory_data
            : cpu.mem_wb.alu_result;

        write_register(cpu.mem_wb.dest, value);

        printf("================ WRITEBACK STAGE ================\n");
        printf("PC                : %u\n", cpu.mem_wb.pc);
        printf("Instruction        : 0x%08X\n", cpu.mem_wb.instruction);

        printf("Destination        : R%u\n", cpu.mem_wb.dest);
        printf("Write Value        : %u\n", value);

        printf("Write Source       : %s\n",
               cpu.mem_wb.control.memToReg
               ? "MEMORY"
               : "ALU");

        printf("=================================================\n");
    }
}

void step_cycle(void) {
    cpu.cycles++;
    printf("--- Clock Cycle %u ---\n", cpu.cycles);

    // LATCH TRANSFERS (At clock edge)
    if (cpu.cycles % 2 == 1 && cpu.cycles > 1) {
        cpu.wb_stage.instruction = cpu.mem_stage.instruction;
        cpu.wb_stage.valid = cpu.mem_stage.valid;
        cpu.mem_stage.valid = false;
    }

    if (cpu.cycles % 2 == 0) {
        cpu.mem_stage.instruction = cpu.ex_stage.instruction;
        cpu.mem_stage.valid = cpu.ex_stage.valid;
        
        cpu.ex_stage.instruction = cpu.id_stage.instruction;
        cpu.ex_stage.valid = cpu.id_stage.valid;
        
        cpu.id_stage.instruction = cpu.if_id.instruction;
        cpu.id_stage.valid = cpu.if_id.valid;
        cpu.if_id.valid = false;
    }

    // RUN STAGES
    fetch_stage();
    decode_stage();
    execute_stage();
    memory_stage();
    writeback_stage();

    if (cpu.pc >= get_instruction_count() && 
        !cpu.if_id.valid && !cpu.id_stage.valid && 
        !cpu.ex_stage.valid && !cpu.mem_stage.valid && 
        !cpu.wb_stage.valid) {
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

