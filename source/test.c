#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define LOAD(...) do {                      \
    data_t prog[] = { __VA_ARGS__ };        \
    memcpy(ram, prog, sizeof(prog));        \
} while (0)

#define TEST_BEGIN(X) do {                  \
    printf("Testing " X " ... ");           \
    fflush(stdout);                         \
    cpu_reset(&cpu);                        \
    memset(ram, 0, RAM_SIZE);               \
    data_bus = 0;                           \
    addr_bus = 0;

#define TEST_END printf("OK\n"); } while(0)

#define TEST_FAIL(REASON, ...) \
    printf("FAIL: " REASON "\n" __VA_OPT__(,) __VA_ARGS__); break

int main(void) {

    /* Init mem */
    data_t ram[RAM_SIZE];
    data_t data_bus;
    addr_t addr_bus;

    /* Init CPU */
    cpu_t cpu;
    cpu.ram = ram;
    cpu.data_bus = &data_bus;
    cpu.addr_bus = &addr_bus;

    TEST_BEGIN("mov");

        LOAD(
            0x7e, 0x00, 0x40, // MOV A, $4000
            0x47,             // MOV B, A
            0x46, 0x00, 0x50, // MOV $5000, B
        );
        ram[0x4000] = 0xaa;

        cpu_step(&cpu);
        if (cpu.A != ram[0x4000]) {
            TEST_FAIL("Load A");
        }

        cpu_step(&cpu);
        if (cpu.B != cpu.A) {
            TEST_FAIL("Store A");
        }

        cpu_step(&cpu);
        if (ram[0x5000] != cpu.B) {
            TEST_FAIL("Store A");
        }

    TEST_END;

}
