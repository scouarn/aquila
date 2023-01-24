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

    /* Output pins: hold/halt, interrupt enabled */
        // FIXME: hold is wrong
    bool hold, inte;

    /* Input pins:
        Setting reset and stepping resets PC, inte, cycles and
        reset is set back to false

        Wait does nothing and is juste an indicator
        set by io which does the waiting */
        //FIXME: ready instead of wait
    bool wait, reset;

    /* Number of cycles from last reset */
    uint64_t cycles;

    /* Set to request an interrupt, instructions
        will be fetched from this pointer */
    data_t *irq_data;
    bool irq_ack; // FIXME

} cpu_state;

/* One instruction */
void cpu_step(void);

#define cpu_A   cpu_state._psw._rp.A
#define cpu_FL  cpu_state._psw._rp.FL
#define cpu_PSW cpu_state._psw.PSW
#define cpu_B   cpu_state._bc._rp.B
#define cpu_C   cpu_state._bc._rp.C
#define cpu_BC  cpu_state._bc.BC
#define cpu_D   cpu_state._de._rp.D
#define cpu_E   cpu_state._de._rp.E
#define cpu_DE  cpu_state._de.DE
#define cpu_H   cpu_state._hl._rp.H
#define cpu_L   cpu_state._hl._rp.L
#define cpu_HL  cpu_state._hl.HL
#define cpu_PC  cpu_state.PC
#define cpu_SP  cpu_state.reg_SP

#define cpu_hold  cpu_state.hold
#define cpu_inte  cpu_state.inte
#define cpu_wait  cpu_state.wait
#define cpu_reset cpu_state.reset

#define cpu_cycles   cpu_state.cycles
#define cpu_irq_data cpu_state.irq_data
#define cpu_irq_ack  cpu_state.irq_ack


#endif
