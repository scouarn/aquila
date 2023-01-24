#include <stdio.h>
#include <unistd.h>

#include "emul/io.h"

data_t io_ram[RAM_SIZE];
data_t io_data_bus;
addr_t io_addr_bus;

data_t io_load(addr_t addr) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return 0x00;
#endif

    return io_ram[addr];
}

void io_store(addr_t addr, data_t data) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return;
#endif
    io_ram[addr] = data;
}

data_t io_input(port_t port) {

    switch(port) {
        case 0: return 0x00;

        case 1: {
            int c = fgetc(stdin);

            if (c == EOF) return 0x00;

            /* Translate LF to CR */
            if (c == '\n') return '\r';

            return c;
        }

        default: return 0x00;
    }

}

void io_output(port_t port, data_t data) {
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

        default: break;
    }
}

