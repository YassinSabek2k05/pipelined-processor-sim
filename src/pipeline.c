#include "../include/pipeline.h"
#include "../include/memory.h"
#include "../include/loader.h"
#include <stdio.h>
#include <string.h>

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

void decode_stage(void) {
    if (!cpu.id_stage.valid) {
        printf("Decode Stage: Idle\n");
        return;
    }
    int stage_cycle = (cpu.cycles % 2 == 0) ? 1 : 2;
    printf("Decode Stage: Decoding instruction 0x%08X (Cycle %d/2)\n",
           cpu.id_stage.instruction, stage_cycle);
}

void execute_stage(void) {
    if (!cpu.ex_stage.valid) {
        printf("Execute Stage: Idle\n");
        return;
    }
    int stage_cycle = (cpu.cycles % 2 == 0) ? 1 : 2;
    printf("Execute Stage: Executing instruction 0x%08X (Cycle %d/2)\n",
           cpu.ex_stage.instruction, stage_cycle);
}

void memory_stage(void) {
    // Structural Hazard: Only MEM access on EVEN cycles
    if (cpu.cycles % 2 != 0 || !cpu.mem_stage.valid) {
        printf("Memory Stage: Idle\n");
        return;
    }
    printf("Memory Stage: Accessing memory for instruction 0x%08X\n", cpu.mem_stage.instruction);
}

void writeback_stage(void) {
    if (cpu.cycles % 2 == 0 || !cpu.wb_stage.valid) {
        printf("Write Back Stage: Idle\n");
        return;
    }
    printf("Write Back Stage: Writing back result for instruction 0x%08X\n", cpu.wb_stage.instruction);
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
