#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <termios.h>
#include <unistd.h>

#include "emul/cpu.h"
#include "emul/io.h"

#include "bdos.h"

#define LOAD_ADDR 0x0100

int main(int argc, char **argv) {

    if (argc != 2) goto usage;

    /* Raw mode */
    struct termios raw, orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    /* Init machine */
    cpu_reset = true;
    cpu_step();
    cpu_PC = LOAD_ADDR;

    /* Load BDOS */
    RAM_LOAD_ARR(0x0000, BDOS);

    /* Open ROM file */
    FILE *fimg = fopen(argv[1], "r");
    if (fimg == NULL) {
        perror("** Cannot load ROM");
        goto usage;
    }
  
    /* Load ROM */
    size_t loaded = RAM_LOAD_FILE(LOAD_ADDR, fimg);
    fprintf(stdout, "** Loaded %zu bytes from %s\r\n", loaded, argv[1]);
    fclose(fimg);

    /* Run */
    while (1) { // TODO: frequency
        cpu_step();

        if (cpu_hold) {
            fprintf(stdout, "** Halted\r\n");
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
