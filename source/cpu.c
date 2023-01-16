#include <assert.h>
#include <stdio.h>

#include "cpu.h"

// https://www.pastraiser.com/cpu/i8080/i8080_opcodes.html
// http://dunfield.classiccmp.org/r/8080.txt
// https://www.autometer.de/unix4fun/z80pack/ftp/manuals/Intel/8080_8085_asm_Nov78.pdf

#define NIMPL assert(0 && "Not implemented")

#define BC  WORD(c->B, c->C)
#define DE  WORD(c->D, c->E)
#define HL  WORD(c->H, c->L)
#define PSW WORD(c->A, c->FL)

#define COND_Z   (c->FL & FLAG_Z)
#define COND_C   (c->FL & FLAG_C)
#define COND_PE  (c->FL & FLAG_P)
#define COND_M   (c->FL & FLAG_S)
#define COND_NZ  (!COND_Z)
#define COND_NC  (!COND_C)
#define COND_PO  (!COND_PE)
#define COND_P   (!COND_M)

/* The bit which is always 1 and the bits which are always 0 */
#define ENFORCE_FLAGS() do {            \
    c->FL |= 2;                         \
    c->FL &= 0xd7;                      \
} while (0)

void cpu_dump(cpu_t *c, FILE *fp) {
    fprintf(fp, "A:  %02x    FL: %02x\n", c->A, c->FL);
    fprintf(fp, "B:  %02x    C:  %02x\n", c->B, c->C);
    fprintf(fp, "D:  %02x    E:  %02x\n", c->D, c->E);
    fprintf(fp, "H:  %02x    L:  %02x\n", c->H, c->L);
    fprintf(fp, "PC: %04x\n", c->PC);
    fprintf(fp, "SP: %04x\n", c->SP);
}

void cpu_reset(cpu_t *c) {
    ENFORCE_FLAGS();
    c->PC = 0;
    c->hold = c->inte = false;
    c->fetch_data = NULL;
    c->cycles = 0;
}

int cpu_irq(cpu_t *c, data_t instruction[]) {
    if (!c->inte) return 0;

    c->inte = false;

    c->fetch_data = instruction;
    cpu_step(c);
    c->fetch_data = NULL;

    return 1;
}

int cpu_irq_rst(cpu_t *c, unsigned int code) {
    if (code > 7) return 0;

    /* RST code */
    data_t instruction = 0xc7 | (code << 3);

    return cpu_irq(c, &instruction);
}

/* Get next byte */
static data_t fetch(cpu_t *c) {

    /* Fetch interrupt instruction */
    if (c->fetch_data != NULL) {
        data_t data = *c->fetch_data;
        c->fetch_data++;
        return data;
    }

    /* Fetch from memory */
    return c->load(c->PC++);
}

/* Get next word */
static addr_t fetch_addr(cpu_t *c) {
    data_t lo = fetch(c);
    data_t hi = fetch(c);
    return WORD(hi, lo);
}

static void set_flag(cpu_t *c, int f, bool b) {
    if (b) c->FL |=  f; // Set the bit to 1
    else   c->FL &= ~f; // Set the bit to 0
}

/* Update the value of zero, sign and parity flag
    based on the value reg (usually the accumulator) */
static void update_ZSP(cpu_t *c, data_t reg) {
    set_flag(c, FLAG_Z, reg == 0x00); // Zero
    set_flag(c, FLAG_S, reg  & 0x80); // Sign

    /* Parity */
    int p = 0;
    for (int mask = (1<<7); mask > 0; mask >>= 1)
        if (reg & mask) p++;

    set_flag(c, FLAG_P, p % 2 == 0);
}

static void push_word(cpu_t *c, addr_t word) {
    c->store(--c->SP, HI_BYTE(word));
    c->store(--c->SP, LO_BYTE(word));
}

static addr_t pop_word(cpu_t *c) {
    data_t lo = c->load(c->SP++);
    data_t hi = c->load(c->SP++);
    return WORD(hi, lo);
}

#define NOP() do { } while (0)

#define MOV_RR(DST, SRC) \
    c->DST = c->SRC;

#define MOV_MR(SRC) \
    c->store(HL, c->SRC);

#define MOV_RM(DEST) \
    c->DEST = c->load(HL);

#define STAX(HI, LO) \
    c->store(WORD(c->HI, c->LO), c->A);

#define STA() \
    c->store(fetch_addr(c), c->A);

#define LDAX(HI, LO) \
    c->A = c->load(WORD(c->HI, c->LO));

#define LDA() \
    c->A = c->load(fetch_addr(c));

#define SHLD() do {                 \
    addr_t addr = fetch_addr(c);    \
    c->store(addr,   c->L);         \
    c->store(addr+1, c->H);         \
} while (0);

#define LHLD() do {                 \
    addr_t addr = fetch_addr(c);    \
    c->L = c->load(addr);           \
    c->H = c->load(addr+1);         \
} while (0);

#define MVI(DST) \
    c->DST = fetch(c);

#define LXI(HI, LO) do {            \
    c->LO = fetch(c);               \
    c->HI = fetch(c);               \
} while (0);

#define LXI_SP() do {               \
    c->SP  = fetch(c);              \
    c->SP |= fetch(c) << 8;         \
} while (0);

#define XCHG() do {                 \
    data_t hi = c->H;               \
    data_t lo = c->L;               \
    c->H = c->D;                    \
    c->L = c->E;                    \
    c->D = hi;                      \
    c->E = lo;                      \
} while (0);

#define XTHL() do {                 \
    addr_t word = pop_word(c);      \
    push_word(c, HL);               \
    c->H = HI_BYTE(word);           \
    c->L = LO_BYTE(word);           \
} while (0);

#define POP(HI, LO) do {            \
    addr_t word = pop_word(c);      \
    c->HI = HI_BYTE(word);          \
    c->LO = LO_BYTE(word);          \
} while (0);

#define POP_PSW() do {              \
    POP(A, FL);                     \
    ENFORCE_FLAGS();                \
} while (0);

/* JMP JZ JNZ JC JNC JPE JPO JM JP */
#define JMP(COND) do {              \
    addr_t addr = fetch_addr(c);    \
    if (!(COND)) break;             \
    c->PC = addr;                   \
} while (0)

/* CALL CZ CNZ CC CNC CPE CPO CM CP */
#define CALL(COND) do {             \
    addr_t addr = fetch_addr(c);    \
    if (!(COND)) break;             \
    c->cycles += 5;                 \
    push_word(c, c->PC);            \
    c->PC = addr;                   \
} while (0)

/* RET RZ RNZ RC RNC RPE RPO RM RP */
#define RET(COND) do {              \
    if (!(COND)) break;             \
    c->cycles += 6;                 \
    c->PC = pop_word(c);            \
} while (0)

#define RST(N) do {                 \
    push_word(c, c->PC);            \
    c->PC = N << 3;                 \
} while (0)

/* INX DCX */
#define INC16(HI, LO, INC) do {     \
    addr_t res = WORD(c->HI, c->LO) + (INC);\
    c->HI = HI_BYTE(res);           \
    c->LO = LO_BYTE(res);           \
} while (0)

#define DAD(DATA) do {              \
    uint32_t res = (DATA) + HL;     \
    c->H = HI_BYTE(res);            \
    c->L = LO_BYTE(res);            \
    set_flag(c, FLAG_C, res > 0xffff);\
} while (0)

#define RLC() do {                  \
    int bit = (c->A >> 7) & 0x01;   \
    c->A = (c->A << 1) | bit;       \
    set_flag(c, FLAG_C, bit);       \
} while (0)

#define RRC() do {                  \
    int bit = c->A & 0x01;          \
    c->A = (c->A >> 1) | (bit << 7);\
    set_flag(c, FLAG_C, bit);       \
} while (0)

#define RAL() do {                  \
    int bit = (c->A >> 7) & 0x01;   \
    c->A = (c->A << 1) | (c->FL & FLAG_C);\
    set_flag(c, FLAG_C, bit);       \
} while (0)

#define RAR() do {                  \
    int bit = c->A & 0x01;          \
    c->A = (c->A >> 1) | ((c->FL & FLAG_C) << 7);\
    set_flag(c, FLAG_C, bit);       \
} while (0)

/* DAA
    I still don't understand how it is supposed to work with the carries
*/
static void daa(cpu_t *c) {

    data_t lo = (c->A & 0x0f);
    data_t hi = (c->A & 0xf0);

    /* Low nibble and half carry */
    if (lo > 0x09 || c->FL & FLAG_A) {
        c->A += 0x06;
        set_flag(c, FLAG_A, lo + 0x06 > 0x0f);
    }

    /* High nibble and normal carry */
    if (hi > 0x90 || c->FL & FLAG_C || (hi >= 0x90 && lo > 0x09)) {
        c->A  += 0x60;
        c->FL |= FLAG_C;
    }

    update_ZSP(c, c->A);
}

/* INR DCR */
static data_t inc8(cpu_t *c, data_t data, int inc) {
    data += inc;
    update_ZSP(c, data);

    if (inc > 0) {
        set_flag(c, FLAG_A, (data & 0x0f) == 0x00);
    }
    else {
        set_flag(c, FLAG_A, (data & 0x0f) != 0x0f);
    }

    return data;
}

/* The 8080 logical AND instructions set the flag to
    reflect the logical OR of bit 3 of the values involved
    in the AND operation.  */
#define AND(DATA) do {              \
    data_t data = DATA;             \
    data_t res  = c->A & data;      \
    update_ZSP(c, res);             \
    set_flag(c, FLAG_A, (c->A | data) & 0x08);\
    set_flag(c, FLAG_C, 0);         \
    c->A = res;                     \
} while (0);

/* ORA ORI XRA XRI */
#define BITWIZE(OP, DATA) do {      \
    c->A = c->A OP (DATA);          \
    update_ZSP(c, c->A);            \
    set_flag(c, FLAG_A, 0);         \
    set_flag(c, FLAG_C, 0);         \
} while (0);

/* ADD ADI ADC ACI */
#define ADC(DATA, CMASK) do {       \
    data_t data = DATA;             \
    uint16_t res = c->A + data + (c->FL & CMASK);\
    uint16_t cr = res ^ data ^ c->A;\
    c->A = res;                     \
    update_ZSP(c, res);             \
    set_flag(c, FLAG_A, cr & 0x010);\
    set_flag(c, FLAG_C, cr & 0x100);\
} while (0);

/* SUB SBI SBB SBI */
#define SBB(DATA, CMASK) do {       \
    data_t data = DATA;             \
    uint16_t res = c->A + ~data + !(c->FL & CMASK);\
    uint16_t cr = res ^ ~data ^ c->A;\
    c->A = res;                     \
    update_ZSP(c, res);             \
    set_flag(c, FLAG_A, cr & 0x010);\
    set_flag(c, FLAG_C,~cr & 0x100);\
} while (0);

/* CMP CPI */
#define CMP(DATA) do {              \
    data_t data = DATA;             \
    uint16_t res = c->A + ~data + 1;\
    uint16_t cr = res ^ ~data ^ c->A;\
    update_ZSP(c, res);             \
    set_flag(c, FLAG_A, cr & 0x010);\
    set_flag(c, FLAG_C,~cr & 0x100);\
} while (0);

const int cycle_table[256] = {
    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
    4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4,
    4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4,

    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,

    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,

    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,
    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,
    5, 10, 10, 18, 11, 11,  7, 11,  5,  5, 10,  5, 11, 17,  7, 11,
    5, 10, 10,  4, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11,
};


void cpu_step(cpu_t *c) {

    /* Check if halted */
    if (c->hold) {
        //c->cycles += 1;
        return;
    }

    /* Fetch */
    uint8_t opc = fetch(c);
    c->cycles += cycle_table[opc];

    /* Decode-Excecute */
    switch (opc) {
        case 0x00: NOP();                               break; // NOP
        case 0x01: LXI(B, C);                           break; // LXI B, d16
        case 0x02: STAX(B, C);                          break; // STAX B
        case 0x03: INC16(B, C, 1);                      break; // INX B
        case 0x04: c->B = inc8(c, c->B,  1);            break; // INR B
        case 0x05: c->B = inc8(c, c->B, -1);            break; // DCR B
        case 0x06: MVI(B);                              break; // MVI B, d8
        case 0x07: RLC();                               break; // RLC
        case 0x08: NOP();                               break; // NOP
        case 0x09: DAD(BC);                             break; // DAD B
        case 0x0a: LDAX(B, C);                          break; // LDAX B
        case 0x0b: INC16(B, C, -1);                     break; // DCX B
        case 0x0c: c->C = inc8(c, c->C,  1);            break; // INR C
        case 0x0d: c->C = inc8(c, c->C, -1);            break; // DCR C
        case 0x0e: MVI(C);                              break; // MVI C, d8
        case 0x0f: RRC();                               break; // RRC
        case 0x10: NOP();                               break; // NOP
        case 0x11: LXI(D, E);                           break; // LXI D, d16
        case 0x12: STAX(D, E);                          break; // STAX D
        case 0x13: INC16(D, E, 1);                      break; // INX D
        case 0x14: c->D = inc8(c, c->D,  1);            break; // INR D
        case 0x15: c->D = inc8(c, c->D, -1);            break; // DCR D
        case 0x16: MVI(D);                              break; // MVI D, d8
        case 0x17: RAL();                               break; // RAL
        case 0x18: NOP();                               break; // NOP
        case 0x19: DAD(DE);                             break; // DAD D
        case 0x1a: LDAX(D, E);                          break; // LDAX D
        case 0x1b: INC16(D, E, -1);                     break; // DCX D
        case 0x1c: c->E = inc8(c, c->E,  1);            break; // INR E
        case 0x1d: c->E = inc8(c, c->E, -1);            break; // DCR E
        case 0x1e: MVI(E);                              break; // MVI E, d8
        case 0x1f: RAR();                               break; // RAR
        case 0x20: NOP();                               break; // NOP
        case 0x21: LXI(H, L);                           break; // LXI H, d16
        case 0x22: SHLD();                              break; // SHLD a16
        case 0x23: INC16(H, L, 1);                      break; // INX H
        case 0x24: c->H = inc8(c, c->H,  1);            break; // INR H
        case 0x25: c->H = inc8(c, c->H, -1);            break; // DCR H
        case 0x26: MVI(H);                              break; // MVI H, d8
        case 0x27: daa(c);                              break; // DAA
        case 0x28: NOP();                               break; // NOP
        case 0x29: DAD(HL);                             break; // DAD H
        case 0x2a: LHLD();                              break; // LHLD a16
        case 0x2b: INC16(H, L, -1);                     break; // DCX H
        case 0x2c: c->L = inc8(c, c->L,  1);            break; // INR L
        case 0x2d: c->L = inc8(c, c->L, -1);            break; // DCR L
        case 0x2e: MVI(L);                              break; // MVI L, d8
        case 0x2f: c->A = ~c->A;                        break; // CMA
        case 0x30: NOP();                               break; // NOP
        case 0x31: LXI_SP();                            break; // LXI SP, d16
        case 0x32: STA();                               break; // STA a16
        case 0x33: c->SP++;                             break; // INX SP
        case 0x34: c->store(HL, inc8(c, c->load(HL),  1)); break; // INR M
        case 0x35: c->store(HL, inc8(c, c->load(HL), -1)); break; // DCR M
        case 0x36: c->store(HL, fetch(c));              break; // MVI M, d8
        case 0x37: c->FL |= FLAG_C;                     break; // STC
        case 0x38: NOP();                               break; // NOP
        case 0x39: DAD(c->SP);                          break; // DAD SP
        case 0x3a: LDA();                               break; // LDA a16
        case 0x3b: c->SP--;                             break; // DCX SP
        case 0x3c: c->A = inc8(c, c->A,  1);            break; // INR A
        case 0x3d: c->A = inc8(c, c->A, -1);            break; // DCR A
        case 0x3e: MVI(A);                              break; // MVI A, d8
        case 0x3f: c->FL ^= FLAG_C;                     break; // CMC
        case 0x40: MOV_RR(B, B);                        break; // MOV B, B
        case 0x41: MOV_RR(B, C);                        break; // MOV B, C
        case 0x42: MOV_RR(B, D);                        break; // MOV B, D
        case 0x43: MOV_RR(B, E);                        break; // MOV B, E
        case 0x44: MOV_RR(B, H);                        break; // MOV B, H
        case 0x45: MOV_RR(B, L);                        break; // MOV B, L
        case 0x46: MOV_RM(B);                           break; // MOV B, M
        case 0x47: MOV_RR(B, A);                        break; // MOV B, A
        case 0x48: MOV_RR(C, B);                        break; // MOV C, B
        case 0x49: MOV_RR(C, C);                        break; // MOV C, C
        case 0x4a: MOV_RR(C, D);                        break; // MOV C, D
        case 0x4b: MOV_RR(C, E);                        break; // MOV C, E
        case 0x4c: MOV_RR(C, H);                        break; // MOV C, H
        case 0x4d: MOV_RR(C, L);                        break; // MOV C, L
        case 0x4e: MOV_RM(C);                           break; // MOV C, M
        case 0x4f: MOV_RR(C, A);                        break; // MOV C, A
        case 0x50: MOV_RR(D, B);                        break; // MOV D, B
        case 0x51: MOV_RR(D, C);                        break; // MOV D, C
        case 0x52: MOV_RR(D, D);                        break; // MOV D, D
        case 0x53: MOV_RR(D, E);                        break; // MOV D, E
        case 0x54: MOV_RR(D, H);                        break; // MOV D, H
        case 0x55: MOV_RR(D, L);                        break; // MOV D, L
        case 0x56: MOV_RM(D);                           break; // MOV D, M
        case 0x57: MOV_RR(D, A);                        break; // MOV D, A
        case 0x58: MOV_RR(E, B);                        break; // MOV E, B
        case 0x59: MOV_RR(E, C);                        break; // MOV E, C
        case 0x5a: MOV_RR(E, D);                        break; // MOV E, D
        case 0x5b: MOV_RR(E, E);                        break; // MOV E, E
        case 0x5c: MOV_RR(E, H);                        break; // MOV E, H
        case 0x5d: MOV_RR(E, L);                        break; // MOV E, L
        case 0x5e: MOV_RM(E);                           break; // MOV E, M
        case 0x5f: MOV_RR(E, A);                        break; // MOV E, A
        case 0x60: MOV_RR(H, B);                        break; // MOV H, B
        case 0x61: MOV_RR(H, C);                        break; // MOV H, C
        case 0x62: MOV_RR(H, D);                        break; // MOV H, D
        case 0x63: MOV_RR(H, E);                        break; // MOV H, E
        case 0x64: MOV_RR(H, H);                        break; // MOV H, H
        case 0x65: MOV_RR(H, L);                        break; // MOV H, L
        case 0x66: MOV_RM(H);                           break; // MOV H, M
        case 0x67: MOV_RR(H, A);                        break; // MOV H, A
        case 0x68: MOV_RR(L, B);                        break; // MOV L, B
        case 0x69: MOV_RR(L, C);                        break; // MOV L, C
        case 0x6a: MOV_RR(L, D);                        break; // MOV L, D
        case 0x6b: MOV_RR(L, E);                        break; // MOV L, E
        case 0x6c: MOV_RR(L, H);                        break; // MOV L, H
        case 0x6d: MOV_RR(L, L);                        break; // MOV L, L
        case 0x6e: MOV_RM(L);                           break; // MOV L, M
        case 0x6f: MOV_RR(L, A);                        break; // MOV L, A
        case 0x70: MOV_MR(B);                           break; // MOV M, B
        case 0x71: MOV_MR(C);                           break; // MOV M, C
        case 0x72: MOV_MR(D);                           break; // MOV M, D
        case 0x73: MOV_MR(E);                           break; // MOV M, E
        case 0x74: MOV_MR(H);                           break; // MOV M, H
        case 0x75: MOV_MR(L);                           break; // MOV M, L
        case 0x76: c->hold = true;                      break; // HLT
        case 0x77: MOV_MR(A);                           break; // MOV M, A
        case 0x78: MOV_RR(A, B);                        break; // MOV A, B
        case 0x79: MOV_RR(A, C);                        break; // MOV A, C
        case 0x7a: MOV_RR(A, D);                        break; // MOV A, D
        case 0x7b: MOV_RR(A, E);                        break; // MOV A, E
        case 0x7c: MOV_RR(A, H);                        break; // MOV A, H
        case 0x7d: MOV_RR(A, L);                        break; // MOV A, L
        case 0x7e: MOV_RM(A);                           break; // MOV A, M
        case 0x7f: MOV_RR(A, A);                        break; // MOV A, A
        case 0x80: ADC(c->B, 0);                        break; // ADD B
        case 0x81: ADC(c->C, 0);                        break; // ADD C
        case 0x82: ADC(c->D, 0);                        break; // ADD D
        case 0x83: ADC(c->E, 0);                        break; // ADD E
        case 0x84: ADC(c->H, 0);                        break; // ADD H
        case 0x85: ADC(c->L, 0);                        break; // ADD L
        case 0x86: ADC(c->load(HL), 0);                 break; // ADD M
        case 0x87: ADC(c->A, 0);                        break; // ADD A
        case 0x88: ADC(c->B, 1);                        break; // ADC B
        case 0x89: ADC(c->C, 1);                        break; // ADC C
        case 0x8a: ADC(c->D, 1);                        break; // ADC D
        case 0x8b: ADC(c->E, 1);                        break; // ADC E
        case 0x8c: ADC(c->H, 1);                        break; // ADC H
        case 0x8d: ADC(c->L, 1);                        break; // ADC L
        case 0x8e: ADC(c->load(HL), 1);                 break; // ADC M
        case 0x8f: ADC(c->A, 1);                        break; // ADC A
        case 0x90: SBB(c->B, 0);                        break; // SUB B
        case 0x91: SBB(c->C, 0);                        break; // SUB C
        case 0x92: SBB(c->D, 0);                        break; // SUB D
        case 0x93: SBB(c->E, 0);                        break; // SUB E
        case 0x94: SBB(c->H, 0);                        break; // SUB H
        case 0x95: SBB(c->L, 0);                        break; // SUB L
        case 0x96: SBB(c->load(HL), 0);                 break; // SUB M
        case 0x97: SBB(c->A, 0);                        break; // SUB A
        case 0x98: SBB(c->B, 1);                        break; // SBB B
        case 0x99: SBB(c->C, 1);                        break; // SBB C
        case 0x9a: SBB(c->D, 1);                        break; // SBB D
        case 0x9b: SBB(c->E, 1);                        break; // SBB E
        case 0x9c: SBB(c->H, 1);                        break; // SBB H
        case 0x9d: SBB(c->L, 1);                        break; // SBB L
        case 0x9e: SBB(c->load(HL), 1);                 break; // SBB M
        case 0x9f: SBB(c->A, 1);                        break; // SBB A
        case 0xa0: AND(c->B);                           break; // ANA B
        case 0xa1: AND(c->C);                           break; // ANA C
        case 0xa2: AND(c->D);                           break; // ANA D
        case 0xa3: AND(c->E);                           break; // ANA E
        case 0xa4: AND(c->H);                           break; // ANA H
        case 0xa5: AND(c->L);                           break; // ANA L
        case 0xa6: AND(c->load(HL));                    break; // ANA M
        case 0xa7: AND(c->A);                           break; // ANA A
        case 0xa8: BITWIZE(^, c->B);                    break; // XRA B
        case 0xa9: BITWIZE(^, c->C);                    break; // XRA C
        case 0xaa: BITWIZE(^, c->D);                    break; // XRA D
        case 0xab: BITWIZE(^, c->E);                    break; // XRA E
        case 0xac: BITWIZE(^, c->H);                    break; // XRA H
        case 0xad: BITWIZE(^, c->L);                    break; // XRA L
        case 0xae: BITWIZE(^, c->load(HL));             break; // XRA M
        case 0xaf: BITWIZE(^, c->A);                    break; // XRA A
        case 0xb0: BITWIZE(|, c->B);                    break; // ORA B
        case 0xb1: BITWIZE(|, c->C);                    break; // ORA C
        case 0xb2: BITWIZE(|, c->D);                    break; // ORA D
        case 0xb3: BITWIZE(|, c->E);                    break; // ORA E
        case 0xb4: BITWIZE(|, c->H);                    break; // ORA H
        case 0xb5: BITWIZE(|, c->L);                    break; // ORA L
        case 0xb6: BITWIZE(|, c->load(HL));             break; // ORA M
        case 0xb7: BITWIZE(|, c->A);                    break; // ORA A
        case 0xb8: CMP(c->B);                           break; // CMP B
        case 0xb9: CMP(c->C);                           break; // CMP C
        case 0xba: CMP(c->D);                           break; // CMP D
        case 0xbb: CMP(c->E);                           break; // CMP E
        case 0xbc: CMP(c->H);                           break; // CMP H
        case 0xbd: CMP(c->L);                           break; // CMP L
        case 0xbe: CMP(c->load(HL));                    break; // CMP M
        case 0xbf: CMP(c->A);                           break; // CMP A
        case 0xc0: RET(COND_NZ);                        break; // RNZ
        case 0xc1: POP(B, C);                           break; // POP B
        case 0xc2: JMP(COND_NZ);                        break; // JNZ a16
        case 0xc3: JMP(true);                           break; // JMP a16
        case 0xc4: CALL(COND_NZ);                       break; // CNZ a16
        case 0xc5: push_word(c, BC);                    break; // PUSH B
        case 0xc6: ADC(fetch(c), 0);                    break; // ADI d8
        case 0xc7: RST(0);                              break; // RST 0
        case 0xc8: RET(COND_Z);                         break; // RZ
        case 0xc9: RET(true);                           break; // RET
        case 0xca: JMP(COND_Z);                         break; // JZ a16
        case 0xcb: JMP(true);                           break; // *JMP a16
        case 0xcc: CALL(COND_Z);                        break; // CZ a16
        case 0xcd: CALL(true);                          break; // CALL a16
        case 0xce: ADC(fetch(c), 1);                    break; // ACI d8
        case 0xcf: RST(1);                              break; // RST 1
        case 0xd0: RET(COND_NC);                        break; // RNC
        case 0xd1: POP(D, E);                           break; // POP D
        case 0xd2: JMP(COND_NC);                        break; // JNC a16
        case 0xd3: c->output(fetch(c), c->A);           break; // OUT p8
        case 0xd4: CALL(COND_NC);                       break; // CNC a16
        case 0xd5: push_word(c, DE);                    break; // PUSH D
        case 0xd6: SBB(fetch(c), 0);                    break; // SUI d8
        case 0xd7: RST(2);                              break; // RST 2
        case 0xd8: RET(COND_C);                         break; // RC
        case 0xd9: RET(true);                           break; // *RET
        case 0xda: JMP(COND_C);                         break; // JC a16
        case 0xdb: c->A = c->input(fetch(c));           break; // IN p8
        case 0xdc: CALL(COND_C);                        break; // CC a16
        case 0xdd: CALL(true);                          break; // *CALL a16
        case 0xde: SBB(fetch(c), 1);                    break; // SBI d8
        case 0xdf: RST(3);                              break; // RST 3
        case 0xe0: RET(COND_PO);                        break; // RPO
        case 0xe1: POP(H, L);                           break; // POP H
        case 0xe2: JMP(COND_PO);                        break; // JPO a16
        case 0xe3: XTHL();                              break; // XTHL
        case 0xe4: CALL(COND_PO);                       break; // CPO a16
        case 0xe5: push_word(c, HL);                    break; // PUSH H
        case 0xe6: AND(fetch(c));                       break; // ANI d8
        case 0xe7: RST(4);                              break; // RST 4
        case 0xe8: RET(COND_PE);                        break; // RPE
        case 0xe9: c->PC = HL;                          break; // PCHL
        case 0xea: JMP(COND_PE);                        break; // JPE a16
        case 0xeb: XCHG();                              break; // XCHG
        case 0xec: CALL(COND_PE);                       break; // CPE a16
        case 0xed: CALL(true);                          break; // *CALL a16
        case 0xee: BITWIZE(^, fetch(c));                break; // XRI d8
        case 0xef: RST(5);                              break; // RST 5
        case 0xf0: RET(COND_P);                         break; // RP
        case 0xf1: POP_PSW();                           break; // POP PSW
        case 0xf2: JMP(COND_P);                         break; // JP a16
        case 0xf3: c->inte = false;                     break; // DI
        case 0xf4: CALL(COND_P);                        break; // CP a16
        case 0xf5: push_word(c, PSW);                   break; // PUSH PSW
        case 0xf6: BITWIZE(|, fetch(c));                break; // ORI d8
        case 0xf7: RST(6);                              break; // RST 6
        case 0xf8: RET(COND_M);                         break; // RM
        case 0xf9: c->SP = HL;                          break; // SPHL
        case 0xfa: JMP(COND_M);                         break; // JM a16
        case 0xfb: c->inte = true;                      break; // EI
        case 0xfc: CALL(COND_M);                        break; // CM a16
        case 0xfd: CALL(true);                          break; // *CALL a16
        case 0xfe: CMP(fetch(c));                       break; // CPI d8
        case 0xff: RST(7);                              break; // RST 7
    }

}
