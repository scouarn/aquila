#include <assert.h>
#include <stdio.h>

#include "cpu.h"

// https://www.pastraiser.com/cpu/i8080/i8080_opcodes.html

#define NIMPL assert(0 && "Not implemented")
#define UNUSED(X) (void)X

void cpu_reset(cpu_t *c) {
    c->PSW = c->BC = c->DE = c->HL = 0;
    c->FL = 2; // The bit which is always 1
    c->PC = c->SP = 0;
    c->halted = c->interrupted = false;
}

void cpu_dump(cpu_t *c, FILE *fp) {
    fprintf(fp, "A:  %02x    FL: %02x\n", c->A, c->FL);
    fprintf(fp, "B:  %02x    C:  %02x\n", c->B, c->C);
    fprintf(fp, "D:  %02x    E:  %02x\n", c->D, c->E);
    fprintf(fp, "H:  %02x    L:  %02x\n", c->H, c->L);
    fprintf(fp, "PC: %04x\n", c->PC);
    fprintf(fp, "SP: %04x\n", c->SP);
}

#define TICK do {           \
    printf("tick\n");       \
} while(0)

static data_t load(cpu_t *c, addr_t addr) {
    *c->addr_bus = addr;
    *c->data_bus = c->ram[addr];
    return *c->data_bus;
}

static void store(cpu_t *c, addr_t addr, data_t data) {
    *c->addr_bus  = addr;
    *c->data_bus  = data;
     c->ram[addr] = data;
    return;
}

static data_t fetch(cpu_t *c) {
    return load(c, c->PC++);
}

static addr_t fetch_addr(cpu_t *c) {
    data_t lo = fetch(c);
    data_t hi = fetch(c);
    return (hi << 8) | lo;
}

#define NOP() do { } while(0)

#define HALT() do {         \
    c->halted = true;       \
} while (0)

#define MOV_RR(D, S) do {               \
    c->D = c->S;                        \
} while(0)

#define MOV_MR(S) do {                  \
    addr_t addr = fetch_addr(c);        \
    store(c, addr, c->S);               \
} while(0)

#define MOV_RM(D) do {                  \
    addr_t addr = fetch_addr(c);        \
    data_t data = load(c, addr);        \
    c->D = data;                        \
} while(0)

void cpu_step(cpu_t *c) {

    /* Check if halted */
    if (c->halted && !c->interrupted) {
        return;
    }

    /* Fetch */
    uint8_t opc = fetch(c);

    /* Decode */
    switch (opc) {
        case 0x00: NOP(); break; /* NOP */
        case 0x01: NIMPL; break; /* */
        case 0x02: NIMPL; break; /* */
        case 0x03: NIMPL; break; /* */
        case 0x04: NIMPL; break; /* */
        case 0x05: NIMPL; break; /* */
        case 0x06: NIMPL; break; /* */
        case 0x07: NIMPL; break; /* */
        case 0x08: NOP(); break; /* NOP */
        case 0x09: NIMPL; break; /* */
        case 0x0a: NIMPL; break; /* */
        case 0x0b: NIMPL; break; /* */
        case 0x0c: NIMPL; break; /* */
        case 0x0d: NIMPL; break; /* */
        case 0x0e: NIMPL; break; /* */
        case 0x0f: NIMPL; break; /* */
        case 0x10: NOP(); break; /* NOP */
        case 0x11: NIMPL; break; /* */
        case 0x12: NIMPL; break; /* */
        case 0x13: NIMPL; break; /* */
        case 0x14: NIMPL; break; /* */
        case 0x15: NIMPL; break; /* */
        case 0x16: NIMPL; break; /* */
        case 0x17: NIMPL; break; /* */
        case 0x18: NOP(); break; /* NOP */
        case 0x19: NIMPL; break; /* */
        case 0x1a: NIMPL; break; /* */
        case 0x1b: NIMPL; break; /* */
        case 0x1c: NIMPL; break; /* */
        case 0x1d: NIMPL; break; /* */
        case 0x1e: NIMPL; break; /* */
        case 0x1f: NIMPL; break; /* */
        case 0x20: NOP(); break; /* NOP */
        case 0x21: NIMPL; break; /* */
        case 0x22: NIMPL; break; /* */
        case 0x23: NIMPL; break; /* */
        case 0x24: NIMPL; break; /* */
        case 0x25: NIMPL; break; /* */
        case 0x26: NIMPL; break; /* */
        case 0x27: NIMPL; break; /* */
        case 0x28: NOP(); break; /* NOP */
        case 0x29: NIMPL; break; /* */
        case 0x2a: NIMPL; break; /* */
        case 0x2b: NIMPL; break; /* */
        case 0x2c: NIMPL; break; /* */
        case 0x2d: NIMPL; break; /* */
        case 0x2e: NIMPL; break; /* */
        case 0x2f: NIMPL; break; /* */
        case 0x30: NOP(); break; /* NOP */
        case 0x31: NIMPL; break; /* */
        case 0x32: NIMPL; break; /* */
        case 0x33: NIMPL; break; /* */
        case 0x34: NIMPL; break; /* */
        case 0x35: NIMPL; break; /* */
        case 0x36: NIMPL; break; /* */
        case 0x37: NIMPL; break; /* */
        case 0x38: NIMPL; break; /* */
        case 0x39: NIMPL; break; /* */
        case 0x3a: NIMPL; break; /* */
        case 0x3b: NIMPL; break; /* */
        case 0x3c: NIMPL; break; /* */
        case 0x3d: NIMPL; break; /* */
        case 0x3e: NIMPL; break; /* */
        case 0x3f: NIMPL; break; /* */
        case 0x40: MOV_RR(B, B); break; /* MOV B, B */
        case 0x41: MOV_RR(B, C); break; /* MOV B, C */
        case 0x42: MOV_RR(B, D); break; /* MOV B, D */
        case 0x43: MOV_RR(B, E); break; /* MOV B, E */
        case 0x44: MOV_RR(B, H); break; /* MOV B, H */
        case 0x45: MOV_RR(B, L); break; /* MOV B, L */
        case 0x46: MOV_RM(B);    break; /* MOV B, mem */
        case 0x47: MOV_RR(B, A); break; /* MOV B, A */
        case 0x48: MOV_RR(C, B); break; /* MOV C, B */
        case 0x49: MOV_RR(C, C); break; /* MOV C, C */
        case 0x4a: MOV_RR(C, D); break; /* MOV C, D */
        case 0x4b: MOV_RR(C, E); break; /* MOV C, E */
        case 0x4c: MOV_RR(C, H); break; /* MOV C, H */
        case 0x4d: MOV_RR(C, L); break; /* MOV C, L */
        case 0x4e: MOV_RM(C);    break; /* MOV C, mem */
        case 0x4f: MOV_RR(C, A); break; /* MOV C, A */
        case 0x50: MOV_RR(D, B); break; /* MOV D, B */
        case 0x51: MOV_RR(D, C); break; /* MOV D, C */
        case 0x52: MOV_RR(D, D); break; /* MOV D, D */
        case 0x53: MOV_RR(D, E); break; /* MOV D, E */
        case 0x54: MOV_RR(D, H); break; /* MOV D, H */
        case 0x55: MOV_RR(D, L); break; /* MOV D, L */
        case 0x56: MOV_RM(D);    break; /* MOV D, mem */
        case 0x57: MOV_RR(D, A); break; /* MOV D, A */
        case 0x58: MOV_RR(E, B); break; /* MOV E, B */
        case 0x59: MOV_RR(E, C); break; /* MOV E, C */
        case 0x5a: MOV_RR(E, D); break; /* MOV E, D */
        case 0x5b: MOV_RR(E, E); break; /* MOV E, E */
        case 0x5c: MOV_RR(E, H); break; /* MOV E, H */
        case 0x5d: MOV_RR(E, L); break; /* MOV E, L */
        case 0x5e: MOV_RM(E);    break; /* MOV E, mem */
        case 0x5f: MOV_RR(E, A); break; /* MOV E, A */
        case 0x60: MOV_RR(H, B); break; /* MOV H, B */
        case 0x61: MOV_RR(H, C); break; /* MOV H, C */
        case 0x62: MOV_RR(H, D); break; /* MOV H, D */
        case 0x63: MOV_RR(H, E); break; /* MOV H, E */
        case 0x64: MOV_RR(H, H); break; /* MOV H, H */
        case 0x65: MOV_RR(H, L); break; /* MOV H, L */
        case 0x66: MOV_RM(H);    break; /* MOV H, mem */
        case 0x67: MOV_RR(H, A); break; /* MOV H, A */
        case 0x68: MOV_RR(L, B); break; /* MOV L, B */
        case 0x69: MOV_RR(L, C); break; /* MOV L, C */
        case 0x6a: MOV_RR(L, D); break; /* MOV L, D */
        case 0x6b: MOV_RR(L, E); break; /* MOV L, E */
        case 0x6c: MOV_RR(L, H); break; /* MOV L, H */
        case 0x6d: MOV_RR(L, L); break; /* MOV L, L */
        case 0x6e: MOV_RM(L);    break; /* MOV L, mem */
        case 0x6f: MOV_RR(L, A); break; /* MOV L, A */
        case 0x70: MOV_MR(B);    break; /* MOV mem, B */
        case 0x71: MOV_MR(C);    break; /* MOV mem, C */
        case 0x72: MOV_MR(D);    break; /* MOV mem, D */
        case 0x73: MOV_MR(E);    break; /* MOV mem, E */
        case 0x74: MOV_MR(H);    break; /* MOV mem, H */
        case 0x75: MOV_MR(L);    break; /* MOV mem, L */
        case 0x76: HALT();       break; /* HLT */
        case 0x77: MOV_MR(A);    break; /* MOV mem, A */
        case 0x78: MOV_RR(A, B); break; /* MOV A, B */
        case 0x79: MOV_RR(A, C); break; /* MOV A, C */
        case 0x7a: MOV_RR(A, D); break; /* MOV A, D */
        case 0x7b: MOV_RR(A, E); break; /* MOV A, E */
        case 0x7c: MOV_RR(A, H); break; /* MOV A, H */
        case 0x7d: MOV_RR(A, L); break; /* MOV A, L */
        case 0x7e: MOV_RM(A);    break; /* MOV A, mem */
        case 0x7f: MOV_RR(A, A); break; /* MOV A, A */
        case 0x80: NIMPL; break; /* */
        case 0x81: NIMPL; break; /* */
        case 0x82: NIMPL; break; /* */
        case 0x83: NIMPL; break; /* */
        case 0x84: NIMPL; break; /* */
        case 0x85: NIMPL; break; /* */
        case 0x86: NIMPL; break; /* */
        case 0x87: NIMPL; break; /* */
        case 0x88: NIMPL; break; /* */
        case 0x89: NIMPL; break; /* */
        case 0x8a: NIMPL; break; /* */
        case 0x8b: NIMPL; break; /* */
        case 0x8c: NIMPL; break; /* */
        case 0x8d: NIMPL; break; /* */
        case 0x8e: NIMPL; break; /* */
        case 0x8f: NIMPL; break; /* */
        case 0x90: NIMPL; break; /* */
        case 0x91: NIMPL; break; /* */
        case 0x92: NIMPL; break; /* */
        case 0x93: NIMPL; break; /* */
        case 0x94: NIMPL; break; /* */
        case 0x95: NIMPL; break; /* */
        case 0x96: NIMPL; break; /* */
        case 0x97: NIMPL; break; /* */
        case 0x98: NIMPL; break; /* */
        case 0x99: NIMPL; break; /* */
        case 0x9a: NIMPL; break; /* */
        case 0x9b: NIMPL; break; /* */
        case 0x9c: NIMPL; break; /* */
        case 0x9d: NIMPL; break; /* */
        case 0x9e: NIMPL; break; /* */
        case 0x9f: NIMPL; break; /* */
        case 0xa0: NIMPL; break; /* */
        case 0xa1: NIMPL; break; /* */
        case 0xa2: NIMPL; break; /* */
        case 0xa3: NIMPL; break; /* */
        case 0xa4: NIMPL; break; /* */
        case 0xa5: NIMPL; break; /* */
        case 0xa6: NIMPL; break; /* */
        case 0xa7: NIMPL; break; /* */
        case 0xa8: NIMPL; break; /* */
        case 0xa9: NIMPL; break; /* */
        case 0xaa: NIMPL; break; /* */
        case 0xab: NIMPL; break; /* */
        case 0xac: NIMPL; break; /* */
        case 0xad: NIMPL; break; /* */
        case 0xae: NIMPL; break; /* */
        case 0xaf: NIMPL; break; /* */
        case 0xb0: NIMPL; break; /* */
        case 0xb1: NIMPL; break; /* */
        case 0xb2: NIMPL; break; /* */
        case 0xb3: NIMPL; break; /* */
        case 0xb4: NIMPL; break; /* */
        case 0xb5: NIMPL; break; /* */
        case 0xb6: NIMPL; break; /* */
        case 0xb7: NIMPL; break; /* */
        case 0xb8: NIMPL; break; /* */
        case 0xb9: NIMPL; break; /* */
        case 0xba: NIMPL; break; /* */
        case 0xbb: NIMPL; break; /* */
        case 0xbc: NIMPL; break; /* */
        case 0xbd: NIMPL; break; /* */
        case 0xbe: NIMPL; break; /* */
        case 0xbf: NIMPL; break; /* */
        case 0xc0: NIMPL; break; /* */
        case 0xc1: NIMPL; break; /* */
        case 0xc2: NIMPL; break; /* */
        case 0xc3: NIMPL; break; /* */
        case 0xc4: NIMPL; break; /* */
        case 0xc5: NIMPL; break; /* */
        case 0xc6: NIMPL; break; /* */
        case 0xc7: NIMPL; break; /* */
        case 0xc8: NIMPL; break; /* */
        case 0xc9: NIMPL; break; /* */
        case 0xca: NIMPL; break; /* */
        case 0xcb: NIMPL; break; /* */
        case 0xcc: NIMPL; break; /* */
        case 0xcd: NIMPL; break; /* */
        case 0xce: NIMPL; break; /* */
        case 0xcf: NIMPL; break; /* */
        case 0xd0: NIMPL; break; /* */
        case 0xd1: NIMPL; break; /* */
        case 0xd2: NIMPL; break; /* */
        case 0xd3: NIMPL; break; /* */
        case 0xd4: NIMPL; break; /* */
        case 0xd5: NIMPL; break; /* */
        case 0xd6: NIMPL; break; /* */
        case 0xd7: NIMPL; break; /* */
        case 0xd8: NIMPL; break; /* */
        case 0xd9: NIMPL; break; /* */
        case 0xda: NIMPL; break; /* */
        case 0xdb: NIMPL; break; /* */
        case 0xdc: NIMPL; break; /* */
        case 0xdd: NIMPL; break; /* */
        case 0xde: NIMPL; break; /* */
        case 0xdf: NIMPL; break; /* */
        case 0xe0: NIMPL; break; /* */
        case 0xe1: NIMPL; break; /* */
        case 0xe2: NIMPL; break; /* */
        case 0xe3: NIMPL; break; /* */
        case 0xe4: NIMPL; break; /* */
        case 0xe5: NIMPL; break; /* */
        case 0xe6: NIMPL; break; /* */
        case 0xe7: NIMPL; break; /* */
        case 0xe8: NIMPL; break; /* */
        case 0xe9: NIMPL; break; /* */
        case 0xea: NIMPL; break; /* */
        case 0xeb: NIMPL; break; /* */
        case 0xec: NIMPL; break; /* */
        case 0xed: NIMPL; break; /* */
        case 0xee: NIMPL; break; /* */
        case 0xef: NIMPL; break; /* */
        case 0xf0: NIMPL; break; /* */
        case 0xf1: NIMPL; break; /* */
        case 0xf2: NIMPL; break; /* */
        case 0xf3: NIMPL; break; /* */
        case 0xf4: NIMPL; break; /* */
        case 0xf5: NIMPL; break; /* */
        case 0xf6: NIMPL; break; /* */
        case 0xf7: NIMPL; break; /* */
        case 0xf8: NIMPL; break; /* */
        case 0xf9: NIMPL; break; /* */
        case 0xfa: NIMPL; break; /* */
        case 0xfb: NIMPL; break; /* */
        case 0xfc: NIMPL; break; /* */
        case 0xfd: NIMPL; break; /* */
        case 0xfe: NIMPL; break; /* */
        case 0xff: NIMPL; break; /* */
        default:   NIMPL; break; /* */
    }

}
