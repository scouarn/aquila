/* CPU interface */
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t addr_t;
typedef uint8_t  data_t;

typedef struct cpu_t {

    /* Accumulator and flags */
    union {
        uint16_t PSW;
        struct { uint8_t A, FL; };
    };

    /* B and C */
    union {
        struct { uint8_t B, C; };
        uint16_t BC;
    };

    /* D and E */
    union {
        struct { uint8_t D, E; };
        uint16_t DE;
    };

    /* H and L */
    union {
        struct { uint8_t H, L; };
        uint16_t HL;
    };

    /* PC and SP */
    uint16_t PC, SP;

    /* Other Internal registers */
    //

    /* Chip interface */
    // bool interrupted, halted;
    void   (*on_write) (struct cpu_t *self, addr_t, data_t);
    data_t (*on_read)  (struct cpu_t *self, addr_t);

} cpu_t;

/* One clock cycle */
void cpu_clock(cpu_t*);

/* One instruction */
void cpu_step(cpu_t*);

/* Reset registers to 0 */
void cpu_reset(cpu_t*);

#endif
