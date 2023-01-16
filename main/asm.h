#ifndef BDOS_H
#define BDOS_H

#include "machine.h"

static data_t BDOS_PROG[] = {

        0x76,               // HLT
        0x00, 0x00,         // NOP
        0x00, 0x00,

// $0005 bdos:              // Entry point for BDOS syscalls
        0xf5,               // PUSH PSW
        0x79,               // MOV A, C
        0xfe, 0x09,         // CPI $09
        0xca, 0x11, 0x00,   // JZ puts

// $000c putchar:           // Syscall 2 : putchar E
        0x7b,               // MOV A, E
        0xd3, TTY_PORT,     // OUT TTY

// $000f end:               // Return from the syscall
        0xf1,               // POP PSW
        0xc9,               // RET

// $0011 puts:              // Syscall 9 : put '$'-terminated string
// $0011 loop:              // If ram[DE++] == '$' then stop else print and loop
        0x1a,               // LDAX D
        0xfe, '$',          // CPI '$'
        0xca, 0x0f, 0x00,   // JZ  end
        0xd3, TTY_PORT,     // OUT TTY
        0x13,               // INX D
        0xc3, 0x11, 0x00,   // JMP loop

// $001C 28 bytes
};

#endif
