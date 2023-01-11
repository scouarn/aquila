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
    data_t A, FL;

    /* General purpose registers */
    data_t B, C, D, E, H, L;

    /* Program counter and stack pointer */
    addr_t PC, SP;

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
