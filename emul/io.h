#ifndef IO_H
#define IO_H

/* Has to be implemented by the application */

#include "cpu.h"

#define SIO_STATUS_PORT 0
#define SIO_DATA_PORT   1
#define SENSE_PORT    255

extern data_t io_ram[];


#define RAM_LOAD_ARR(OFF, ARR) do {    \
    memcpy(io_ram+OFF, ARR, sizeof(ARR));      \
} while (0)

#define RAM_LOAD(OFF, ...) do {        \
    const data_t prog[] = { __VA_ARGS__ };  \
    RAM_LOAD_ARR(OFF, prog);           \
} while (0)

#define RAM_LOAD_FILE(OFF, FP) \
    fread(io_ram+OFF, 1, RAM_SIZE-OFF, FP);


/* Memory read/write */
data_t io_load   (addr_t addr);
void   io_store  (addr_t addr, data_t data);

/* Port read/write */
data_t io_input  (port_t port);
void   io_output (port_t port, data_t data);

#endif
