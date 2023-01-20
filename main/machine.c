#include <stdio.h>
#include <unistd.h>

#include "machine.h"

cpu_t  cpu;
data_t ram[RAM_SIZE];

data_t load(addr_t addr) {
    return ram[addr];
}

void store(addr_t addr, data_t data) {
    ram[addr] = data;
}

data_t input(port_t port) {

    switch(port) {
        case 0: return 0x02;

        case 1: {
            int c = fgetc(stdin);

            if (c == EOF) return 0x00;

            /* Translate LF to CR */
            if (c == '\n') return '\r';

            return c;
        }

        default:
            //printf("In %d\n", port);
        return 0x00;
    }

}

void output(port_t port, data_t data) {
    switch(port) {

        case 1:
            if (isatty(STDOUT_FILENO)) {
                fputc(data & 0x7f, stdout);
                fflush(stdout);
            }
            else {
                fputc(data, stdout);
            }
        break;

        default:
            //printf("Out %d\n", port);
        break;
    }
}

