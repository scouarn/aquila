#include <assert.h>

#include "cpu.h"

void cpu_reset(cpu_t *c) {
    c->PSW = c->BC = c->DE = c->HL = 0;
    c->PC  = c->SP = 0;
}

void cpu_clock(cpu_t *c) {
    (void)c;
    assert(0 && "Not implemented");
}

void cpu_step(cpu_t *c) {
    (void)c;
    assert(0 && "Not implemented");
}
