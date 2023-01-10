#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"

typedef struct {
    data_t ram[RAM_SIZE];
    cpu_t cpu;

    addr_t addr_bus;
    data_t data_bus;

} mac_t;

void mac_init(mac_t*);
void mac_reset(mac_t*);

void mac_step(mac_t *m);

void mac_dep(mac_t*, data_t);
void mac_dep_next(mac_t*, data_t);

void mac_ex(mac_t*, addr_t);
void mac_ex_next(mac_t*);

#endif
