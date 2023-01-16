#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include "emul/cpu.h"
#include "asm.h"
#include "machine.h"

#define LOAD_ADDR 0x0100

int main(int argc, char **argv) {

    if (argc != 2) goto usage;

    /* Init machine */
    cpu.load   = load;
    cpu.store  = store;
    cpu.input  = input;
    cpu.output = output;
    cpu_reset(&cpu);

    RAM_LOAD_ARR(ram, 0x0000, BDOS_PROG);
    //ram[0] = 0x76; // HLT
    cpu.PC = LOAD_ADDR;

    FILE *fimg = fopen(argv[1], "r");
    if (fimg == NULL) {
        perror("** Cannot load initial orders");
        goto usage;
    }

    size_t loaded = RAM_LOAD_FILE(ram, LOAD_ADDR, fimg);
    fprintf(stderr, "** Loaded %zu bytes from %s\n", loaded, argv[1]);
    fclose(fimg);

    while (1) { // TODO: frequency
        cpu_step(&cpu);

        if (cpu.hold) {
            fprintf(stderr, "** Halted\n");
            break;
        }
    }

    return 0;

usage:
    fprintf(stderr,
        "Usage: %s <mem_image>\n",
    argv[0]);

    return 1;
}
