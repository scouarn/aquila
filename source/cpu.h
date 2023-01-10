/* CPU interface */
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define RAM_SIZE 0x10000

typedef uint16_t addr_t;
typedef uint8_t  data_t;

/* State of a 8080 CPU */
typedef struct cpu_t {

    /* Acc and flags */
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

    /* Program counter and stack pointer */
    uint16_t PC, SP;

    bool halted, interrupted;

    /* Memory */
    data_t *ram;
    addr_t *addr_bus;
    data_t *data_bus;

} cpu_t;

/* Display registers */
void cpu_dump(cpu_t *c, FILE *fp);

/* One instruction */
void cpu_step(cpu_t*);

/* Reset registers */
void cpu_reset(cpu_t*);

#endif
