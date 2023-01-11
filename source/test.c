#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define LOAD(...) do {                      \
    data_t prog[] = { __VA_ARGS__ };        \
    memcpy(ram, prog, sizeof(prog));        \
} while (0)

#define TEST_BEGIN(X) do {                  \
    printf("Testing %-8s", X);           \
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

    TEST_BEGIN("MOV");
        LOAD(
            0x7e, // MOV A, M
            0x47, // MOV B, A
            0x70, // MOV M, B
        );

        ram[0x4000] = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("MOV A, M : A=$%02x", cpu.A);
        }

        cpu_step(&cpu);
        if (cpu.B != 0xaa) {
            TEST_FAIL("MOV B, A : B=$%02x", cpu.B);
        }

        cpu.H = 0x50; cpu.L = 0x00;
        cpu_step(&cpu);
        if (ram[0x5000] != 0xaa) {
            TEST_FAIL("MOV M, B : M=$%02x", ram[0x5000]);
        }

    TEST_END;

    TEST_BEGIN("STAX");
        LOAD(
            0x02, // STAX B
            0x12, // STAX D
        );

        cpu.A = 0xaa;
        cpu.B = 0x40; cpu.C = 0x00;
        cpu_step(&cpu);
        if (ram[0x4000] != 0xaa) {
            TEST_FAIL("STAX B : M=$%02x", ram[0x4000]);
        }

        cpu.A = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        if (ram[0x5000] != 0xbb) {
            TEST_FAIL("STAX D : M=$%02x", ram[0x5000]);
        }

    TEST_END;

    TEST_BEGIN("STA");
        LOAD(
            0x32, 0x00, 0x40, // STA $4000
        );

        cpu.A = 0xaa;
        cpu_step(&cpu);
        if (ram[0x4000] != 0xaa) {
            TEST_FAIL("STA : M=$%02x", ram[0x4000]);
        }
    TEST_END;

    TEST_BEGIN("LDAX");
        LOAD(
            0x0a, // LDAX B
            0x1a, // LDAX D
        );

        ram[0x4000] = 0xaa;
        cpu.B = 0x40; cpu.C = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("LDAX B : A=$%02x", cpu.A);
        }

        ram[0x5000] = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0xbb) {
            TEST_FAIL("LDAX D : A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("LDA");
        LOAD(
            0x3A, 0x00, 0x40, // LDA $4000
        );

        ram[0x4000] = 0xaa;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("LTA : A=$%02x", cpu.A);
        }
    TEST_END;
}
