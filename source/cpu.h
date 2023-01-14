#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define HI_BYTE(X) ((X >> 8) & 0xff)
#define LO_BYTE(X) (X & 0xff)
#define WORD(H, L) ((H << 8) | L)

typedef uint16_t addr_t;
typedef uint8_t  data_t;
typedef uint8_t  port_t;

/* State of a 8080 CPU */
typedef struct cpu_t {

    /* Acc and flags */
    data_t A, FL;

    /* General purpose registers */
    data_t B, C, D, E, H, L;

    /* Program counter and stack pointer */
    addr_t PC, SP;

    bool hold, inte;

    /* Memory and IO */
    void   (*store)  (addr_t, data_t);
    void   (*output) (port_t, data_t);
    data_t (*load)   (addr_t);
    data_t (*input)  (port_t);

} cpu_t;

/* Display registers */
void cpu_dump(cpu_t *c, FILE *fp);

/* One instruction, return the number of cycles */
int cpu_step(cpu_t*);

/* Only reset PC and hold/inte */
void cpu_reset(cpu_t*);

/* Send itnerrupt signal */
void cpu_interrupt(cpu_t*);

#endif
