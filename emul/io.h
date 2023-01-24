#ifndef IO_H
#define IO_H

/* Has to be implemented by the application */

#include "cpu.h"

#ifndef RAM_SIZE
#define RAM_SIZE  0x10000
#endif

#define MAX_PORT  256

/* Ports the CPU will use to communicate via serial */
#define SIO_STATUS 0
#define SIO_DATA   1

/* Baud rate used by the serial port */
#define BAUD_RATE 9600U
#define BAUD_PRESCALLER ((F_CPU / (16UL*BAUD_RATE))-1)

/* Big chunk of memory */
extern data_t io_ram[RAM_SIZE];

/* Current state of the buses */
extern data_t io_data_bus;
extern addr_t io_addr_bus;


#define RAM_LOAD_ARR(OFF, ARR) do {    \
    memcpy(io_ram+OFF, ARR, sizeof(ARR));      \
} while (0)

#define RAM_LOAD(OFF, ...) do {        \
    const data_t prog[] = { __VA_ARGS__ };  \
    RAM_LOAD_ARR(OFF, prog);           \
} while (0)

#define RAM_LOAD_FILE(OFF, FP) \
    fread(io_ram+OFF, 1, sizeof(io_ram)-OFF, FP);


/* Memory read/write */
data_t io_load   (addr_t addr);
void   io_store  (addr_t addr, data_t data);

/* Port read/write */
data_t io_input  (port_t port);
void   io_output (port_t port, data_t data);

/* Setup the serial port */
void io_serial_init(void);

#endif
