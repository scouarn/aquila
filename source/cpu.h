#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t addr_t;
typedef uint8_t  data_t;
typedef uint8_t  port_t;

#define HI_BYTE(X) ((X >> 8) & 0xff)
#define LO_BYTE(X) (X & 0xff)
#define WORD(H, L) ((H << 8) | L)

#define RAM_SIZE 0x10000
#define MAX_PORT 256

#define RAM_LOAD(RAM, OFF, ...) do {        \
    const data_t prog[] = { __VA_ARGS__ };  \
    memcpy(RAM+OFF, prog, sizeof(prog));    \
} while (0)

#define RAM_LOAD_FILE(RAM, OFF, FP) \
    fread(RAM+OFF, 1, sizeof(RAM)-OFF, FP);

#define FLAG_C 0x01
#define FLAG_P 0x04
#define FLAG_A 0x10
#define FLAG_Z 0x40
#define FLAG_S 0x80

/* State of a 8080 CPU */
typedef struct cpu_t {

    /* Acc and flags */
    data_t A, FL;

    /* General purpose registers */
    data_t B, C, D, E, H, L;

    /* Program counter and stack pointer */
    addr_t PC, SP;

    /* hold/halt and interrupt enable bits */
    bool hold, inte;

    /* If not NULL, instructions will be fetched from this pointer,
        used for interrupt requests */
    data_t *fetch_data;

    /* Number of cycles from last cpu_reset */
    uint64_t cycles;

    /* Memory and IO */
    void   (*store)  (addr_t, data_t);
    void   (*output) (port_t, data_t);
    data_t (*load)   (addr_t);
    data_t (*input)  (port_t);

} cpu_t;

/* Display registers */
void cpu_dump(cpu_t *c, FILE *fp);

/* One instruction */
void cpu_step(cpu_t*);

/* Only reset PC and hold/inte */
void cpu_reset(cpu_t*);

/* Send interrupt with data for one instruction wich is executed,
    return 1 if the irq was acknoledged */
int cpu_irq(cpu_t*, data_t instruction[]);

/* Send interrupt with a RST instruction,
    the code must be <= 7,
    return 1 if the irq was acknoledged */
int cpu_irq_rst(cpu_t*, unsigned int code);

#endif
