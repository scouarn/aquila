#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "ram.h"

// https://altairclone.com/downloads/cpu_tests/

#define LOAD_ADDR 0x0100

static cpu_t cpu;
static data_t ram[RAM_SIZE];

static data_t load(addr_t addr) {
    return ram[addr];
}

static void store(addr_t addr, data_t data) {
    ram[addr] = data;
}

static data_t input(port_t port) {
    (void)port;
    return 0x00;
/*
    switch (port) {

        case TTY_PORT: {
            int c = fgetc(tty_fp);
            if (c == EOF) return 0x00;
            else return (data_t)c;
        } break;

        default: return 0x00;
    }
*/
}

static void output(port_t port, data_t data) {

    if (port != 1) return;

    switch (cpu.C) {

    case 2: // print E
        fputc(data, stdout);
    break;

    case 9: { // dump mem from DE to $ char
        addr_t addr = WORD(cpu.D, cpu.E);
        data_t chr;

        while ((chr = load(addr++)) != '$') {
            fputc(chr, stdout);
        }

        fflush(stdout);
    } break;

    default: break;
    }
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s program.com\n", argv[0]);
        return 1;
    }

    cpu.load   = load;
    cpu.store  = store;
    cpu.input  = input;
    cpu.output = output;
    cpu_reset(&cpu);

    cpu.PC = LOAD_ADDR;
    cpu.SP = 0xffff;

    RAM_LOAD(ram, 0x0000,
        0x76 // HLT
    );

    RAM_LOAD(ram, 0x0005,
        0xd3, 0x01, // OUT 1
        0xc9,       // RET
    );

/*
        xchg
loop:   mov a, m
        cmp to $
        ret if $
        out 1
        inx h
        jmp loop
*/

    FILE *fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("Open test rom");
        return 1;
    }

    size_t loaded = RAM_LOAD_FILE(ram, LOAD_ADDR, fin);
    printf("==> Loaded %zu bytes from %s\n", loaded, argv[1]);

    unsigned long cycle = 0;
    while (!cpu.hold) {
        cycle += cpu_step(&cpu);
    }

    printf("\n\n==> Test done: %lu steps\n", cycle);
    return 0;
}
