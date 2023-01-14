#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
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
    cpu_hard_reset(&cpu);

    cpu.PC = LOAD_ADDR;
    cpu.SP = 0xffff;

    memset(ram, 0, sizeof(ram));

    ram[0x0000] = 0x76; // HLT
    ram[0x0005] = 0xd3; // OUT
    ram[0x0006] = 0x01; // 0x01
    ram[0x0007] = 0xc9; // RET
/*
        xchg
loop:   mov a, m
        cmp to $
        ret if $
        out 1
        inx h
        jmp loop
*/

/*
    ram[0x00] = 0xc3; // JMP
    ram[0x01] = LO_BYTE(LOAD_ADDR);
    ram[0x02] = HI_BYTE(LOAD_ADDR);
*/

    FILE *fin = fopen(argv[1], "rb");
    size_t loaded = fread(ram + LOAD_ADDR, 1, sizeof(ram) - LOAD_ADDR, fin);
    printf("==> Loaded %zu bytes from %s\n", loaded, argv[1]);

    unsigned long cycle = 0;
    while (!cpu.hold) {
        cpu_step(&cpu);
        cycle++;
    }

    printf("\n\n==> Test done: %lu steps\n", cycle);
    cpu_dump(&cpu, stdout);

    return 0;
}
