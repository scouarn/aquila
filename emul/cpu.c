#include <assert.h>
#include <stdio.h>

#include "cpu.h"
#include "io.h"

// https://www.pastraiser.com/cpu/i8080/i8080_opcodes.html
// http://dunfield.classiccmp.org/r/8080.txt
// https://www.autometer.de/unix4fun/z80pack/ftp/manuals/Intel/8080_8085_asm_Nov78.pdf

struct s_cpu_regs cpu_regs;
bool cpu_hold, cpu_inte;
uint64_t cycles;

/* If not NULL, instructions will be fetched from this pointer,
    used for interrupt requests */
static data_t *fetch_data;

#define NIMPL assert(0 && "Not implemented")

/* Conditions */
#define COND_Z   (cpu_FL & FLAG_Z)
#define COND_C   (cpu_FL & FLAG_C)
#define COND_PE  (cpu_FL & FLAG_P)
#define COND_M   (cpu_FL & FLAG_S)
#define COND_NZ  (!COND_Z)
#define COND_NC  (!COND_C)
#define COND_PO  (!COND_PE)
#define COND_P   (!COND_M)

void cpu_reset(void) {
    ENFORCE_FLAGS();
    cpu_PC = 0;
    cpu_hold = cpu_inte = false;
    fetch_data = NULL;
    cycles = 0;
}

int cpu_irq(data_t instruction[]) {
    if (!cpu_inte) return 0;

    cpu_inte = false;

    fetch_data = instruction;
    cpu_step();
    fetch_data = NULL;

    return 1;
}

int cpu_irq_rst(uint8_t code) {
    if (code > 7) return 0;

    /* RST code */
    data_t instruction = 0xc7 | (code << 3);

    return cpu_irq(&instruction);
}

/* Get next byte */
static data_t fetch() {

    /* Fetch interrupt instruction */
    if (fetch_data != NULL) {
        data_t data = *fetch_data;
        fetch_data++;
        return data;
    }

    /* Fetch from memory */
    return io_load(cpu_PC++);
}

/* Get next word */
static addr_t fetch_addr(void) {
    data_t lo = fetch();
    data_t hi = fetch();
    return WORD(hi, lo);
}

static void set_flag(int f, bool b) {
    if (b) cpu_FL |=  f; // Set the bit to 1
    else   cpu_FL &= ~f; // Set the bit to 0
}

/* Update the value of zero, sign and parity flag
    based on the value reg (usually the accumulator) */
static void update_ZSP(data_t reg) {
    set_flag(FLAG_Z, reg == 0x00); // Zero
    set_flag(FLAG_S, reg  & 0x80); // Sign

    /* Parity */
    int p = 0;
    for (int mask = (1<<7); mask > 0; mask >>= 1)
        if (reg & mask) p++;

    set_flag(FLAG_P, p % 2 == 0);
}

static void push_word(addr_t word) {
    io_store(--cpu_SP, HI_BYTE(word));
    io_store(--cpu_SP, LO_BYTE(word));
}

static addr_t pop_word(void) {
    data_t lo = io_load(cpu_SP++);
    data_t hi = io_load(cpu_SP++);
    return WORD(hi, lo);
}

#define NOP() do { } while (0)

#define MOV_MR(SRC) \
    io_store(cpu_HL, SRC);

#define MOV_RM(DEST) \
    DEST = io_load(cpu_HL);

#define STAX(ADDR) \
    io_store(ADDR, cpu_A);

#define LDAX(RP) \
    cpu_A = io_load(RP);

#define LDA() \
    cpu_A = io_load(fetch_addr());

#define SHLD() do {                 \
    addr_t addr = fetch_addr();     \
    io_store(addr,   cpu_L);        \
    io_store(addr+1, cpu_H);        \
} while (0);

#define LHLD() do {                 \
    addr_t addr = fetch_addr();     \
    cpu_L = io_load(addr);          \
    cpu_H = io_load(addr+1);        \
} while (0);

#define MVI(DST) \
    DST = fetch();

#define LXI(RP) do {                \
    RP  = fetch();                  \
    RP |= fetch() << 8;             \
} while (0);

#define XCHG() do {                 \
    addr_t tmp = cpu_HL;            \
    cpu_HL = cpu_DE;                \
    cpu_DE = tmp;                   \
} while (0);

#define XTHL() do {                 \
    addr_t word = pop_word();       \
    push_word(cpu_HL);              \
    cpu_HL = word;                  \
} while (0);

#define POP(RP) \
    RP = pop_word();

/* JMP JZ JNZ JC JNC JPE JPO JM JP */
#define JMP(COND) do {              \
    addr_t addr = fetch_addr();     \
    if (!(COND)) break;             \
    cpu_PC = addr;                  \
} while (0)

/* CALL CZ CNZ CC CNC CPE CPO CM CP */
#define CALL(COND) do {             \
    addr_t addr = fetch_addr();     \
    if (!(COND)) break;             \
    cycles += 5;                    \
    push_word(cpu_PC);              \
    cpu_PC = addr;                  \
} while (0)

/* RET RZ RNZ RC RNC RPE RPO RM RP */
#define RET(COND) do {              \
    if (!(COND)) break;             \
    cycles += 6;                    \
    cpu_PC = pop_word();            \
} while (0)

#define RST(N) do {                 \
    push_word(cpu_PC);              \
    cpu_PC = N << 3;                \
} while (0)

/* Cannot test greater than 0xffff on the hardware... */
#define DAD(DATA) do {              \
    uint32_t data = DATA;           \
    uint32_t res = data + cpu_HL;   \
    set_flag(FLAG_C, res > 0xffff); \
    cpu_HL = res;                   \
} while (0)

#define RLC() do {                  \
    int bit = (cpu_A >> 7) & 0x01;  \
    cpu_A = (cpu_A << 1) | bit;     \
    set_flag(FLAG_C, bit);          \
} while (0)

#define RRC() do {                  \
    int bit = cpu_A & 0x01;         \
    cpu_A = (cpu_A >> 1) | (bit << 7);\
    set_flag(FLAG_C, bit);          \
} while (0)

#define RAL() do {                  \
    int bit = (cpu_A >> 7) & 0x01;  \
    cpu_A = (cpu_A << 1) | (cpu_FL & FLAG_C);\
    set_flag(FLAG_C, bit);          \
} while (0)

#define RAR() do {                  \
    int bit = cpu_A & 0x01;         \
    cpu_A = (cpu_A >> 1) | ((cpu_FL & FLAG_C) << 7);\
    set_flag(FLAG_C, bit);          \
} while (0)

/* DAA
    I still don't understand how it is supposed to work with the carries
*/
static inline void daa(void) {

    data_t lo = (cpu_A & 0x0f);
    data_t hi = (cpu_A & 0xf0);

    /* Low nibble and half carry */
    if (lo > 0x09 || cpu_FL & FLAG_A) {
        cpu_A += 0x06;
        set_flag(FLAG_A, lo + 0x06 > 0x0f);
    }

    /* High nibble and normal carry */
    if (hi > 0x90 || cpu_FL & FLAG_C || (hi >= 0x90 && lo > 0x09)) {
        cpu_A  += 0x60;
        cpu_FL |= FLAG_C;
    }

    update_ZSP(cpu_A);
}

static data_t inc8(data_t data, int inc) {
    data += inc;
    update_ZSP(data);

    if (inc > 0) {
        set_flag(FLAG_A, (data & 0x0f) == 0x00);
    }
    else {
        set_flag(FLAG_A, (data & 0x0f) != 0x0f);
    }

    return data;
}

#define INR(REG) \
    REG = inc8(REG, 1);

#define DCR(REG) \
    REG = inc8(REG, -1);

/* The 8080 logical AND instructions set the flag to
    reflect the logical OR of bit 3 of the values involved
    in the AND operation.  */
static void AND(data_t data) {
    data_t res = cpu_A & data;
    update_ZSP(res);
    set_flag(FLAG_A, (cpu_A | data) & 0x08);\
    set_flag(FLAG_C, 0);
    cpu_A = res;
}

/* XRA XRI */
static void XRA(data_t data) {
    cpu_A = cpu_A ^ data;
    update_ZSP(cpu_A);
    set_flag(FLAG_A, 0);
    set_flag(FLAG_C, 0);
}

/* ORA ORI */
static void ORA(data_t data) {
    cpu_A = cpu_A | data;
    update_ZSP(cpu_A);
    set_flag(FLAG_A, 0);
    set_flag(FLAG_C, 0);
}

/* ADD ADI ADC ACI */
static void ADC(data_t data, uint8_t cmask) {
    uint16_t res = cpu_A + data + (cpu_FL & cmask);
    uint16_t cr = res ^ data ^ cpu_A;
    cpu_A = res;
    update_ZSP(res);
    set_flag(FLAG_A, cr & 0x010);
    set_flag(FLAG_C, cr & 0x100);
}

/* SUB SBI SBB SBI */
static void SBB(data_t data, uint8_t cmask) {
    uint16_t res = cpu_A + ~data + !(cpu_FL & cmask);
    uint16_t cr = res ^ ~data ^ cpu_A;
    cpu_A = res;
    update_ZSP(res);
    set_flag(FLAG_A, cr & 0x010);
    set_flag(FLAG_C,~cr & 0x100);
}

/* CMP CPI */
static void CMP(data_t data) {
    uint16_t res = cpu_A + ~data + 1;
    uint16_t cr = res ^ ~data ^ cpu_A;
    update_ZSP(res);
    set_flag(FLAG_A, cr & 0x010);
    set_flag(FLAG_C,~cr & 0x100);
}

const uint8_t cycle_table[256] = {
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


void cpu_step(void) {

    /* Check if halted */
    if (cpu_hold) {
        //cycles += 1;
        return;
    }

    /* Fetch */
    uint8_t opc = fetch();
    cycles += cycle_table[opc];

    /* Decode-Excecute */
    switch (opc) {
        case 0x00: NOP();                               break; // NOP
        case 0x01: LXI(cpu_BC);                         break; // LXI B, d16
        case 0x02: STAX(cpu_BC);                        break; // STAX B
        case 0x03: cpu_BC++;                            break; // INX B
        case 0x04: INR(cpu_B);                          break; // INR B
        case 0x05: DCR(cpu_B);                          break; // DCR B
        case 0x06: MVI(cpu_B);                          break; // MVI B, d8
        case 0x07: RLC();                               break; // RLC
        case 0x08: NOP();                               break; // NOP
        case 0x09: DAD(cpu_BC);                         break; // DAD B
        case 0x0a: LDAX(cpu_BC);                        break; // LDAX B
        case 0x0b: cpu_BC--;                            break; // DCX B
        case 0x0c: INR(cpu_C);                          break; // INR C
        case 0x0d: DCR(cpu_C);                          break; // DCR C
        case 0x0e: MVI(cpu_C);                          break; // MVI C, d8
        case 0x0f: RRC();                               break; // RRC
        case 0x10: NOP();                               break; // NOP
        case 0x11: LXI(cpu_DE);                         break; // LXI D, d16
        case 0x12: STAX(cpu_DE);                        break; // STAX D
        case 0x13: cpu_DE++;                            break; // INX D
        case 0x14: INR(cpu_D);                          break; // INR D
        case 0x15: DCR(cpu_D);                          break; // DCR D
        case 0x16: MVI(cpu_D);                          break; // MVI D, d8
        case 0x17: RAL();                               break; // RAL
        case 0x18: NOP();                               break; // NOP
        case 0x19: DAD(cpu_DE);                         break; // DAD D
        case 0x1a: LDAX(cpu_DE);                        break; // LDAX D
        case 0x1b: cpu_DE--;                            break; // DCX D
        case 0x1c: INR(cpu_E);                          break; // INR E
        case 0x1d: DCR(cpu_E);                          break; // DCR E
        case 0x1e: MVI(cpu_E);                          break; // MVI E, d8
        case 0x1f: RAR();                               break; // RAR
        case 0x20: NOP();                               break; // NOP
        case 0x21: LXI(cpu_HL);                         break; // LXI H, d16
        case 0x22: SHLD();                              break; // SHLD a16
        case 0x23: cpu_HL++;                            break; // INX H
        case 0x24: INR(cpu_H);                          break; // INR H
        case 0x25: DCR(cpu_H);                          break; // DCR H
        case 0x26: MVI(cpu_H);                          break; // MVI H, d8
        case 0x27: daa();                               break; // DAA
        case 0x28: NOP();                               break; // NOP
        case 0x29: DAD(cpu_HL);                         break; // DAD H
        case 0x2a: LHLD();                              break; // LHLD a16
        case 0x2b: cpu_HL--;                            break; // DCX H
        case 0x2c: INR(cpu_L);                          break; // INR L
        case 0x2d: DCR(cpu_L);                          break; // DCR L
        case 0x2e: MVI(cpu_L);                          break; // MVI L, d8
        case 0x2f: cpu_A = ~cpu_A;                      break; // CMA
        case 0x30: NOP();                               break; // NOP
        case 0x31: LXI(cpu_SP);                         break; // LXI SP, d16
        case 0x32: STAX(fetch_addr());                  break; // STA a16
        case 0x33: cpu_SP++;                            break; // INX SP
        case 0x34: io_store(cpu_HL, inc8(io_load(cpu_HL),  1)); break; // INR M
        case 0x35: io_store(cpu_HL, inc8(io_load(cpu_HL), -1)); break; // DCR M
        case 0x36: io_store(cpu_HL, fetch());           break; // MVI M, d8
        case 0x37: cpu_FL |= FLAG_C;                    break; // STC
        case 0x38: NOP();                               break; // NOP
        case 0x39: DAD(cpu_SP);                         break; // DAD SP
        case 0x3a: LDA();                               break; // LDA a16
        case 0x3b: cpu_SP--;                            break; // DCX SP
        case 0x3c: INR(cpu_A);                          break; // INR A
        case 0x3d: DCR(cpu_A);                          break; // DCR A
        case 0x3e: MVI(cpu_A);                          break; // MVI A, d8
        case 0x3f: cpu_FL ^= FLAG_C;                    break; // CMC
        case 0x40: cpu_B = cpu_B;                       break; // MOV B, B
        case 0x41: cpu_B = cpu_C;                       break; // MOV B, C
        case 0x42: cpu_B = cpu_D;                       break; // MOV B, D
        case 0x43: cpu_B = cpu_E;                       break; // MOV B, E
        case 0x44: cpu_B = cpu_H;                       break; // MOV B, H
        case 0x45: cpu_B = cpu_L;                       break; // MOV B, L
        case 0x46: MOV_RM(cpu_B);                       break; // MOV B, M
        case 0x47: cpu_B = cpu_A;                       break; // MOV B, A
        case 0x48: cpu_C = cpu_B;                       break; // MOV C, B
        case 0x49: cpu_C = cpu_C;                       break; // MOV C, C
        case 0x4a: cpu_C = cpu_D;                       break; // MOV C, D
        case 0x4b: cpu_C = cpu_E;                       break; // MOV C, E
        case 0x4c: cpu_C = cpu_H;                       break; // MOV C, H
        case 0x4d: cpu_C = cpu_L;                       break; // MOV C, L
        case 0x4e: MOV_RM(cpu_C);                       break; // MOV C, M
        case 0x4f: cpu_C = cpu_A;                       break; // MOV C, A
        case 0x50: cpu_D = cpu_B;                       break; // MOV D, B
        case 0x51: cpu_D = cpu_C;                       break; // MOV D, C
        case 0x52: cpu_D = cpu_D;                       break; // MOV D, D
        case 0x53: cpu_D = cpu_E;                       break; // MOV D, E
        case 0x54: cpu_D = cpu_H;                       break; // MOV D, H
        case 0x55: cpu_D = cpu_L;                       break; // MOV D, L
        case 0x56: MOV_RM(cpu_D);                       break; // MOV D, M
        case 0x57: cpu_D = cpu_A;                       break; // MOV D, A
        case 0x58: cpu_E = cpu_B;                       break; // MOV E, B
        case 0x59: cpu_E = cpu_C;                       break; // MOV E, C
        case 0x5a: cpu_E = cpu_D;                       break; // MOV E, D
        case 0x5b: cpu_E = cpu_E;                       break; // MOV E, E
        case 0x5c: cpu_E = cpu_H;                       break; // MOV E, H
        case 0x5d: cpu_E = cpu_L;                       break; // MOV E, L
        case 0x5e: MOV_RM(cpu_E);                       break; // MOV E, M
        case 0x5f: cpu_E = cpu_A;                       break; // MOV E, A
        case 0x60: cpu_H = cpu_B;                       break; // MOV H, B
        case 0x61: cpu_H = cpu_C;                       break; // MOV H, C
        case 0x62: cpu_H = cpu_D;                       break; // MOV H, D
        case 0x63: cpu_H = cpu_E;                       break; // MOV H, E
        case 0x64: cpu_H = cpu_H;                       break; // MOV H, H
        case 0x65: cpu_H = cpu_L;                       break; // MOV H, L
        case 0x66: MOV_RM(cpu_H);                       break; // MOV H, M
        case 0x67: cpu_H = cpu_A;                       break; // MOV H, A
        case 0x68: cpu_L = cpu_B;                       break; // MOV L, B
        case 0x69: cpu_L = cpu_C;                       break; // MOV L, C
        case 0x6a: cpu_L = cpu_D;                       break; // MOV L, D
        case 0x6b: cpu_L = cpu_E;                       break; // MOV L, E
        case 0x6c: cpu_L = cpu_H;                       break; // MOV L, H
        case 0x6d: cpu_L = cpu_L;                       break; // MOV L, L
        case 0x6e: MOV_RM(cpu_L);                       break; // MOV L, M
        case 0x6f: cpu_L = cpu_A;                       break; // MOV L, A
        case 0x70: MOV_MR(cpu_B);                       break; // MOV M, B
        case 0x71: MOV_MR(cpu_C);                       break; // MOV M, C
        case 0x72: MOV_MR(cpu_D);                       break; // MOV M, D
        case 0x73: MOV_MR(cpu_E);                       break; // MOV M, E
        case 0x74: MOV_MR(cpu_H);                       break; // MOV M, H
        case 0x75: MOV_MR(cpu_L);                       break; // MOV M, L
        case 0x76: cpu_hold = true;                     break; // HLT
        case 0x77: MOV_MR(cpu_A);                       break; // MOV M, A
        case 0x78: cpu_A = cpu_B;                       break; // MOV A, B
        case 0x79: cpu_A = cpu_C;                       break; // MOV A, C
        case 0x7a: cpu_A = cpu_D;                       break; // MOV A, D
        case 0x7b: cpu_A = cpu_E;                       break; // MOV A, E
        case 0x7c: cpu_A = cpu_H;                       break; // MOV A, H
        case 0x7d: cpu_A = cpu_L;                       break; // MOV A, L
        case 0x7e: MOV_RM(cpu_A);                       break; // MOV A, M
        case 0x7f: cpu_A = cpu_A;                       break; // MOV A, A
        case 0x80: ADC(cpu_B, 0);                       break; // ADD B
        case 0x81: ADC(cpu_C, 0);                       break; // ADD C
        case 0x82: ADC(cpu_D, 0);                       break; // ADD D
        case 0x83: ADC(cpu_E, 0);                       break; // ADD E
        case 0x84: ADC(cpu_H, 0);                       break; // ADD H
        case 0x85: ADC(cpu_L, 0);                       break; // ADD L
        case 0x86: ADC(io_load(cpu_HL), 0);             break; // ADD M
        case 0x87: ADC(cpu_A, 0);                       break; // ADD A
        case 0x88: ADC(cpu_B, 1);                       break; // ADC B
        case 0x89: ADC(cpu_C, 1);                       break; // ADC C
        case 0x8a: ADC(cpu_D, 1);                       break; // ADC D
        case 0x8b: ADC(cpu_E, 1);                       break; // ADC E
        case 0x8c: ADC(cpu_H, 1);                       break; // ADC H
        case 0x8d: ADC(cpu_L, 1);                       break; // ADC L
        case 0x8e: ADC(io_load(cpu_HL), 1);             break; // ADC M
        case 0x8f: ADC(cpu_A, 1);                       break; // ADC A
        case 0x90: SBB(cpu_B, 0);                       break; // SUB B
        case 0x91: SBB(cpu_C, 0);                       break; // SUB C
        case 0x92: SBB(cpu_D, 0);                       break; // SUB D
        case 0x93: SBB(cpu_E, 0);                       break; // SUB E
        case 0x94: SBB(cpu_H, 0);                       break; // SUB H
        case 0x95: SBB(cpu_L, 0);                       break; // SUB L
        case 0x96: SBB(io_load(cpu_HL), 0);             break; // SUB M
        case 0x97: SBB(cpu_A, 0);                       break; // SUB A
        case 0x98: SBB(cpu_B, 1);                       break; // SBB B
        case 0x99: SBB(cpu_C, 1);                       break; // SBB C
        case 0x9a: SBB(cpu_D, 1);                       break; // SBB D
        case 0x9b: SBB(cpu_E, 1);                       break; // SBB E
        case 0x9c: SBB(cpu_H, 1);                       break; // SBB H
        case 0x9d: SBB(cpu_L, 1);                       break; // SBB L
        case 0x9e: SBB(io_load(cpu_HL), 1);             break; // SBB M
        case 0x9f: SBB(cpu_A, 1);                       break; // SBB A
        case 0xa0: AND(cpu_B);                          break; // ANA B
        case 0xa1: AND(cpu_C);                          break; // ANA C
        case 0xa2: AND(cpu_D);                          break; // ANA D
        case 0xa3: AND(cpu_E);                          break; // ANA E
        case 0xa4: AND(cpu_H);                          break; // ANA H
        case 0xa5: AND(cpu_L);                          break; // ANA L
        case 0xa6: AND(io_load(cpu_HL));                break; // ANA M
        case 0xa7: AND(cpu_A);                          break; // ANA A
        case 0xa8: XRA(cpu_B);                          break; // XRA B
        case 0xa9: XRA(cpu_C);                          break; // XRA C
        case 0xaa: XRA(cpu_D);                          break; // XRA D
        case 0xab: XRA(cpu_E);                          break; // XRA E
        case 0xac: XRA(cpu_H);                          break; // XRA H
        case 0xad: XRA(cpu_L);                          break; // XRA L
        case 0xae: XRA(io_load(cpu_HL));                break; // XRA M
        case 0xaf: XRA(cpu_A);                          break; // XRA A
        case 0xb0: ORA(cpu_B);                          break; // ORA B
        case 0xb1: ORA(cpu_C);                          break; // ORA C
        case 0xb2: ORA(cpu_D);                          break; // ORA D
        case 0xb3: ORA(cpu_E);                          break; // ORA E
        case 0xb4: ORA(cpu_H);                          break; // ORA H
        case 0xb5: ORA(cpu_L);                          break; // ORA L
        case 0xb6: ORA(io_load(cpu_HL));                break; // ORA M
        case 0xb7: ORA(cpu_A);                          break; // ORA A
        case 0xb8: CMP(cpu_B);                          break; // CMP B
        case 0xb9: CMP(cpu_C);                          break; // CMP C
        case 0xba: CMP(cpu_D);                          break; // CMP D
        case 0xbb: CMP(cpu_E);                          break; // CMP E
        case 0xbc: CMP(cpu_H);                          break; // CMP H
        case 0xbd: CMP(cpu_L);                          break; // CMP L
        case 0xbe: CMP(io_load(cpu_HL));                break; // CMP M
        case 0xbf: CMP(cpu_A);                          break; // CMP A
        case 0xc0: RET(COND_NZ);                        break; // RNZ
        case 0xc1: POP(cpu_BC);                         break; // POP B
        case 0xc2: JMP(COND_NZ);                        break; // JNZ a16
        case 0xc3: JMP(true);                           break; // JMP a16
        case 0xc4: CALL(COND_NZ);                       break; // CNZ a16
        case 0xc5: push_word(cpu_BC);                   break; // PUSH B
        case 0xc6: ADC(fetch(), 0);                     break; // ADI d8
        case 0xc7: RST(0);                              break; // RST 0
        case 0xc8: RET(COND_Z);                         break; // RZ
        case 0xc9: RET(true);                           break; // RET
        case 0xca: JMP(COND_Z);                         break; // JZ a16
        case 0xcb: JMP(true);                           break; // *JMP a16
        case 0xcc: CALL(COND_Z);                        break; // CZ a16
        case 0xcd: CALL(true);                          break; // CALL a16
        case 0xce: ADC(fetch(), 1);                     break; // ACI d8
        case 0xcf: RST(1);                              break; // RST 1
        case 0xd0: RET(COND_NC);                        break; // RNC
        case 0xd1: POP(cpu_DE);                         break; // POP D
        case 0xd2: JMP(COND_NC);                        break; // JNC a16
        case 0xd3: io_output(fetch(), cpu_A);           break; // OUT p8
        case 0xd4: CALL(COND_NC);                       break; // CNC a16
        case 0xd5: push_word(cpu_DE);                   break; // PUSH D
        case 0xd6: SBB(fetch(), 0);                     break; // SUI d8
        case 0xd7: RST(2);                              break; // RST 2
        case 0xd8: RET(COND_C);                         break; // RC
        case 0xd9: RET(true);                           break; // *RET
        case 0xda: JMP(COND_C);                         break; // JC a16
        case 0xdb: cpu_A = io_input(fetch());           break; // IN p8
        case 0xdc: CALL(COND_C);                        break; // CC a16
        case 0xdd: CALL(true);                          break; // *CALL a16
        case 0xde: SBB(fetch(), 1);                     break; // SBI d8
        case 0xdf: RST(3);                              break; // RST 3
        case 0xe0: RET(COND_PO);                        break; // RPO
        case 0xe1: POP(cpu_HL);                         break; // POP H
        case 0xe2: JMP(COND_PO);                        break; // JPO a16
        case 0xe3: XTHL();                              break; // XTHL
        case 0xe4: CALL(COND_PO);                       break; // CPO a16
        case 0xe5: push_word(cpu_HL);                   break; // PUSH H
        case 0xe6: AND(fetch());                        break; // ANI d8
        case 0xe7: RST(4);                              break; // RST 4
        case 0xe8: RET(COND_PE);                        break; // RPE
        case 0xe9: cpu_PC = cpu_HL;                     break; // PCHL
        case 0xea: JMP(COND_PE);                        break; // JPE a16
        case 0xeb: XCHG();                              break; // XCHG
        case 0xec: CALL(COND_PE);                       break; // CPE a16
        case 0xed: CALL(true);                          break; // *CALL a16
        case 0xee: XRA(fetch());                        break; // XRI d8
        case 0xef: RST(5);                              break; // RST 5
        case 0xf0: RET(COND_P);                         break; // RP
        case 0xf1: POP(cpu_PSW); ENFORCE_FLAGS();       break; // POP PSW
        case 0xf2: JMP(COND_P);                         break; // JP a16
        case 0xf3: cpu_inte = false;                    break; // DI
        case 0xf4: CALL(COND_P);                        break; // CP a16
        case 0xf5: push_word(cpu_PSW);                  break; // PUSH PSW
        case 0xf6: ORA(fetch());                        break; // ORI d8
        case 0xf7: RST(6);                              break; // RST 6
        case 0xf8: RET(COND_M);                         break; // RM
        case 0xf9: cpu_SP = cpu_HL;                     break; // SPHL
        case 0xfa: JMP(COND_M);                         break; // JM a16
        case 0xfb: cpu_inte = true;                     break; // EI
        case 0xfc: CALL(COND_M);                        break; // CM a16
        case 0xfd: CALL(true);                          break; // *CALL a16
        case 0xfe: CMP(fetch());                        break; // CPI d8
        case 0xff: RST(7);                              break; // RST 7
    }

}
