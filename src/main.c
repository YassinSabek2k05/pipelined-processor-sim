#include "../include/gui.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "program.txt";
    run_gui(filename);
    return EXIT_SUCCESS;
}