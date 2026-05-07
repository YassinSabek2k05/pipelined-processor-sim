#include "../include/loader.h"
#include "../include/memory.h"
#include "../include/pipeline.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "program.txt";

    init_memory();

    if (!load_program(filename)) {
        return EXIT_FAILURE;
    }

    int instruction_count = get_instruction_count();

    printf("Loaded %d instructions into memory", instruction_count);

    if (instruction_count > 0) {
        printf(" (addresses 0-%d)\n", instruction_count - 1);
    } else {
        printf(" (no instructions loaded)\n");
    }

    printf("Data segment starts at address %d\n", DATA_SEGMENT_START);

    init_processor();
    run_simulation();

    return EXIT_SUCCESS;
}