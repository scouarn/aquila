#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <termios.h>
#include <unistd.h>

#include "emul/cpu.h"
#include "machine.h"

#define LOAD_ADDR 0x0000

int main(int argc, char **argv) {

    if (argc != 2) goto usage;

    /* Raw mode */
    struct termios raw, orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    /* Init machine */
    cpu.load   = load;
    cpu.store  = store;
    cpu.input  = input;
    cpu.output = output;
    cpu_reset(&cpu);
    cpu.PC = LOAD_ADDR;

    /* Open ROM file */
    FILE *fimg = fopen(argv[1], "r");
    if (fimg == NULL) {
        perror("** Cannot load ROM");
        goto usage;
    }

    /* Load ROM */
    size_t loaded = RAM_LOAD_FILE(ram, LOAD_ADDR, fimg);
    fprintf(stderr, "** Loaded %zu bytes from %s\n", loaded, argv[1]);
    fclose(fimg);

    /* Run */
    while (1) { // TODO: frequency
        fprintf(stderr, "PC $%04x\n", cpu.PC);
        cpu_step(&cpu);

        if (cpu.hold) {
            fprintf(stderr, "** Halted\n");
            break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    return 0;

usage:
    fprintf(stderr,
        "Usage: %s <rom_image>\n",
    argv[0]);

    return 1;
}
