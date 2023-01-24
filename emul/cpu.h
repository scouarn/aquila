#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t addr_t;
typedef uint8_t  data_t;
typedef uint8_t  port_t;

#define HI_BYTE(X) ((data_t)(X >> 8))
#define LO_BYTE(X) ((data_t)X)
#define WORD(H, L) ((addr_t)(H << 8) | L)

#define FLAG_C (1<<0)
#define FLAG_P (1<<2)
#define FLAG_A (1<<4)
#define FLAG_Z (1<<6)
#define FLAG_S (1<<7)

/* The bit which is always 1 and the bits which are always 0 */
#define ENFORCE_FLAGS() do {            \
    cpu_FL |= 2;                        \
    cpu_FL &= 0xd7;                     \
} while (0)


/* Register bank assuming little endian */
extern struct s_cpu_regs {

    union { /* A and FL */
        addr_t PSW;
        struct { data_t FL, A; } _rp;
    } _psw;

    union { /* B and C */
        addr_t BC;
        struct { data_t C, B; } _rp;
    } _bc;

    union { /* D and E */
        addr_t DE;
        struct { data_t E, D; } _rp;
    } _de;

    union { /* H and L */
        addr_t HL;
        struct { data_t L, H; } _rp;
    } _hl;

    /* Program counter and stack pointer */
    addr_t PC, reg_SP; // SP is a defined macro

} cpu_regs;

#define cpu_A   cpu_regs._psw._rp.A
#define cpu_FL  cpu_regs._psw._rp.FL
#define cpu_PSW cpu_regs._psw.PSW
#define cpu_B   cpu_regs._bc._rp.B
#define cpu_C   cpu_regs._bc._rp.C
#define cpu_BC  cpu_regs._bc.BC
#define cpu_D   cpu_regs._de._rp.D
#define cpu_E   cpu_regs._de._rp.E
#define cpu_DE  cpu_regs._de.DE
#define cpu_H   cpu_regs._hl._rp.H
#define cpu_L   cpu_regs._hl._rp.L
#define cpu_HL  cpu_regs._hl.HL
#define cpu_PC  cpu_regs.PC
#define cpu_SP  cpu_regs.reg_SP


/* hold/halt and interrupt enable bits */
extern bool cpu_hold, cpu_inte;

/* Number of cycles from last cpu_reset */
extern uint64_t cycles;


/* One instruction */
void cpu_step(void);

/* Only reset PC and hold/inte */
void cpu_reset(void);

/* Send interrupt with data for one instruction wich is executed,
    return 1 if the irq was acknoledged */
int cpu_irq(data_t instruction[]);

/* Send interrupt with a RST instruction,
    the code must be <= 7,
    return 1 if the irq was acknoledged */
int cpu_irq_rst(uint8_t code);

#endif
