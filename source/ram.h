#ifndef RAM_H
#define RAM_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define RAM_SIZE 0x10000

#define RAM_LOAD(RAM, OFF, ...) do {        \
    const data_t prog[] = { __VA_ARGS__ };  \
    memcpy(RAM+OFF, prog, sizeof(prog));    \
} while (0)

#define RAM_LOAD_FILE(RAM, OFF, FP) \
    fread(RAM+OFF, 1, sizeof(RAM)-OFF, FP);

#endif
