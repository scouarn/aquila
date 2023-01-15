#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

// https://altairclone.com/downloads/cpu_tests/

#define LOAD_ADDR 0x0100
#define TTY_PORT  1

static cpu_t cpu;
static data_t ram[RAM_SIZE];

static data_t load(addr_t addr) {
    return ram[addr];
}

static void store(addr_t addr, data_t data) {
    ram[addr] = data;
}

static data_t input(port_t port) {
    switch (port) {

        case TTY_PORT: {
            int c = fgetc(stdin);
            if (c == EOF) return 0x00;
            else return (data_t)c;
        } break;

        default: return 0x00;
    }
}

static void output(port_t port, data_t data) {
    switch (port) {

        case TTY_PORT:
            fputc(data, stdout);
            fflush(stdout);
        break;

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

    /* BDOS syscall 2 and 9 */
    RAM_LOAD(ram, 0x0000,
        0x76,               // HLT
        0x00, 0x00,         // NOP
        0x00, 0x00,

        // $0005 bdos:      // Entry point for BDOS syscalls
        0xf5,               // PUSH PSW
        0xe5,               // PUSH HL
        0x79,               // MOV A, C
        0xfe, 0x02,         // CPI $02
        0xcc, 0x15, 0x00,   // CZ putchar
        0xfe, 0x09,         // CPI $09
        0xcc, 0x19, 0x00,   // CZ puts
        0xe1,               // POP HL
        0xf1,               // POP PSW
        0xc9,               // RET

        // $0015 putchar:   // Syscall 2 : putchar E
        0x7b,               // MOV A, E
        0xd3, TTY_PORT,     // OUT TTY
        0xc9,               // RET

        // $0019 puts:      // Syscall 9 : put string DE terminated by '$'
        0xeb,               // XCHG : move DE in HL
        // $001a loop:
        0x7e,               // MOV A, M
        0xfe, '$',          // CPI '$'
        0xc8,               // RZ
        0xd3, TTY_PORT,     // OUT TTY
        0x23,               // INX H
        0xc3, 0x1a, 0x00,   // JMP loop
    );

    FILE *fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("Open test rom");
        return 1;
    }

    size_t loaded = RAM_LOAD_FILE(ram, LOAD_ADDR, fin);
    fclose(fin);
    printf("==> Loaded %zu bytes from %s\n", loaded, argv[1]);

    unsigned long cycle = 0;
    while (!cpu.hold) {
        cycle += cpu_step(&cpu);
    }

    printf("\n\n==> Test done: %lu cycles\n", cycle);
    return 0;
}
