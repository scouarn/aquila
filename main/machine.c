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
        case TTY_PORT: {
            int c = fgetc(stdin);
            return c != EOF ? c : 0x00;
        }

        default: return 0x00;
    }

}

void output(port_t port, data_t data) {
    switch(port) {
        case TTY_PORT:
            fputc(data, stdout);
            if (isatty(STDOUT_FILENO))
                fflush(stdout);
        break;

        default: break;
    }
}

