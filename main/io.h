#ifndef MACHINE_H
#define MACHINE_H

#include "emul/cpu.h"

#define TTY_PORT 0

/* Defining the machine */
extern cpu_t cpu;
extern data_t ram[RAM_SIZE];

/* Mem read CPU callback */
data_t load(addr_t addr);

/* Mem write CPU callback */
void store(addr_t addr, data_t data);

/* IO read CPU callback */
data_t input(port_t port);

/* IO write CPU callback */
void output(port_t port, data_t data);

#endif
