#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "machine.h"

void mac_init(mac_t *m) {
    mac_reset(m);
    memset(m->ram, 0, RAM_SIZE * sizeof(data_t));
}

void mac_reset(mac_t *m) {
    cpu_reset(&m->cpu);
    m->addr_bus = 0;
    m->data_bus = 0;
}

void mac_step(mac_t *m) {
    cpu_step(&m->cpu);
}

void mac_dep(mac_t *m, data_t data) {
    m->data_bus = data;
    m->ram[m->addr_bus] = m->data_bus;
}

void mac_dep_next(mac_t *m, data_t data) {
    m->addr_bus++;
    mac_dep(m, data);
}

void mac_ex(mac_t *m, addr_t addr) {
    m->addr_bus = addr;
    m->data_bus = m->ram[m->addr_bus];
}

void mac_ex_next(mac_t *m) {
    mac_ex(m, m->addr_bus + 1);
}
