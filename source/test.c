#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define LOAD_AT(OFF, ...) do {              \
    data_t prog[] = { __VA_ARGS__ };        \
    memcpy(ram+OFF, prog, sizeof(prog));    \
} while (0)

#define LOAD(...) LOAD_AT(0, __VA_ARGS__)

#define TEST_BEGIN(X) do {                  \
    printf("Testing %-10s", X);             \
    fflush(stdout);                         \
    cpu_reset(&cpu);                        \
    memset(ram, 0, RAM_SIZE);               \
    data_bus = 0;                           \
    addr_bus = 0;

#define TEST_END \
    printf("\033[32mOK\033[0m\n"); \
} while(0)

#define TEST_FAIL(REASON, ...) \
    printf("\033[31mFAIL\033[0m: " REASON "\n" __VA_OPT__(,) __VA_ARGS__); \
    break

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

    TEST_BEGIN("MOV"); // TODO: more tests for these instructions
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
            TEST_FAIL("B : M=$%02x", ram[0x4000]);
        }

        cpu.A = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        if (ram[0x5000] != 0xbb) {
            TEST_FAIL("D : M=$%02x", ram[0x5000]);
        }

    TEST_END;

    TEST_BEGIN("STA");
        LOAD(
            0x32, 0x00, 0x40, // STA $4000
        );

        cpu.A = 0xaa;
        cpu_step(&cpu);
        if (ram[0x4000] != 0xaa) {
            TEST_FAIL("M=$%02x", ram[0x4000]);
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
            TEST_FAIL("B : A=$%02x", cpu.A);
        }

        ram[0x5000] = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0xbb) {
            TEST_FAIL("D : A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("LDA");
        LOAD(
            0x3a, 0x00, 0x40, // LDA $4000
        );

        ram[0x4000] = 0xaa;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("A=$%02x", cpu.A);
        }
    TEST_END;

    TEST_BEGIN("SHLD");
        LOAD(
            0x22, 0x00, 0x40, // SHLD $4000
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu_step(&cpu);
        if (ram[0x4000] != 0xbb) {
            TEST_FAIL("M=$%02x", ram[0x4000]);
        }
        if (ram[0x4001] != 0xaa) {
            TEST_FAIL("M'=$%02x", ram[0x4001]);
        }

    TEST_END;

    TEST_BEGIN("LHLD");
        LOAD(
            0x2a, 0x00, 0x40, // LHLD $4000
        );

        ram[0x4000] = 0xaa;
        ram[0x4001] = 0xbb;
        cpu_step(&cpu);
        if (cpu.H != 0xbb) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xaa) {
            TEST_FAIL("L=$%02x", cpu.L);
        }

    TEST_END;

    TEST_BEGIN("MVI");
        LOAD(
            0x06, 0xbb, // MVI B, $bb
            0x0e, 0xcc, // MVI C, $cc
            0x16, 0xdd, // MVI D, $dd
            0x1e, 0xee, // MVI E, $ee
            0x26, 0xff, // MVI H, $ff
            0x2e, 0x11, // MVI L, $11
            0x36, 0x99, // MVI M, $99
            0x3e, 0xaa, // MVI A, $aa
        );

        cpu_step(&cpu);
        if (cpu.B != 0xbb) {
            TEST_FAIL("B=$%02x", cpu.B);
        }

        cpu_step(&cpu);
        if (cpu.C != 0xcc) {
            TEST_FAIL("C=$%02x", cpu.C);
        }

        cpu_step(&cpu);
        if (cpu.D != 0xdd) {
            TEST_FAIL("B=$%02x", cpu.D);
        }

        cpu_step(&cpu);
        if (cpu.E != 0xee) {
            TEST_FAIL("E=$%02x", cpu.E);
        }

        cpu_step(&cpu);
        if (cpu.H != 0xff) {
            TEST_FAIL("H=$%02x", cpu.H);
        }

        cpu_step(&cpu);
        if (cpu.L != 0x11) {
            TEST_FAIL("L=$%02x", cpu.L);
        }

        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        if (ram[0x4000] != 0x99) {
            TEST_FAIL("M=$%02x", ram[0x4000]);
        }

        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("LXI");
        LOAD(
            0x01, 0xcc, 0xbb, // LXI B, $bbcc
            0x11, 0xee, 0xdd, // LXI D, $ddee
            0x21, 0x11, 0xff, // LXI H, $ff11
            0x31, 0x44, 0x33, // LXI SP,$3344
        );

        cpu_step(&cpu);
        if (cpu.B != 0xbb) {
            TEST_FAIL("B=$%02x", cpu.B);
        }
        if (cpu.C != 0xcc) {
            TEST_FAIL("C=$%02x", cpu.C);
        }

        cpu_step(&cpu);
        if (cpu.D != 0xdd) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.E != 0xee) {
            TEST_FAIL("E=$%02x", cpu.E);
        }

        cpu_step(&cpu);
        if (cpu.H != 0xff) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0x11) {
            TEST_FAIL("L=$%02x", cpu.L);
        }

        cpu_step(&cpu);
        if (cpu.SP != 0x3344) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("XCHG");
        LOAD(
            0xeb // XCHG
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu.D = 0xdd; cpu.E = 0xee;
        cpu_step(&cpu);
        if (cpu.H != 0xdd) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xee) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.D != 0xaa) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.E != 0xbb) {
            TEST_FAIL("E=$%02x", cpu.E);
        }

    TEST_END;

    TEST_BEGIN("SPHL");
        LOAD(
            0xf9 // SPHL
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu_step(&cpu);
        if (cpu.SP != 0xaabb) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("PUSH"); // TODO: test with other registers
        LOAD(
            0xd5, // PUSH D
        );

        cpu.D = 0x8f; cpu.E = 0x9d;
        cpu.SP = 0x3a2c;
        cpu_step(&cpu);
        if (ram[0x3a2b] != 0x8f) {
            TEST_FAIL("M=$%02x", ram[0x3a2b]);
        }
        if (ram[0x3a2a] != 0x9d) {
            TEST_FAIL("M'=$%02x", ram[0x3a2a]);
        }
        if (cpu.SP != 0x3a2a) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("POP"); // TODO: test with other registers
        LOAD(
            0xe1, // POP H
        );

        ram[0x1239] = 0x3d;
        ram[0x123A] = 0x93;
        cpu.SP = 0x1239;

        cpu_step(&cpu);
        if (cpu.H != 0x93) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0x3D) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.SP != 0x123b) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("XTHL");
        LOAD(
            0xe3, // XTHL
        );

        cpu.SP = 0x10ad;
        cpu.H = 0x0b;
        cpu.L = 0x3c;
        ram[0x10ad] = 0xf0;
        ram[0x10ae] = 0x0d;

        cpu_step(&cpu);
        if (cpu.H != 0x0d) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xf0) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (ram[0x10ae] != 0x0b) {
            TEST_FAIL("M=$%02x", ram[0x10ae]);
        }
        if (ram[0x10ad] != 0x3c) {
            TEST_FAIL("M'=$%02x", ram[0x10ad]);
        }
        if (cpu.SP != 0x10ad) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("EI-DI");
        LOAD(
            0xf3, // DI
            0xfb, // EI
        );

        if (cpu.interrupt_enabled != true) {
            TEST_FAIL("Initial value of INTE=%d", cpu.interrupt_enabled);
        }

        cpu_step(&cpu);
        if (cpu.interrupt_enabled != false) {
            TEST_FAIL("DI INTE=%d", cpu.interrupt_enabled);
        }

        cpu_step(&cpu);
        if (cpu.interrupt_enabled != true) {
            TEST_FAIL("EI INTE=%d", cpu.interrupt_enabled);
        }

    TEST_END;

    TEST_BEGIN("STC-CMC");
        LOAD(
            0x37, // STC
            0x3f, // CMC
        );

        if (cpu.FL != 0x02) { // The bit which is alway 1
            TEST_FAIL("FL=$%02x", cpu.FL);
        }

        cpu_step(&cpu);
        if (cpu.FL != 0x03) {
            TEST_FAIL("FL'=$%02x", cpu.FL);
        }

        cpu_step(&cpu);
        if (cpu.FL != 0x02) {
            TEST_FAIL("FL''=$%02x", cpu.FL);
        }

    TEST_END;

    TEST_BEGIN("CMA");
        LOAD(
            0x2f, // CMA
        );

        cpu.A = 0x55;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("DAA");
        LOAD(
            0x27, // DAA
        );

        cpu.A = 0x9b;
        cpu_step(&cpu);
        if (cpu.A != 0x01) {
            TEST_FAIL("A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x13) {
            TEST_FAIL("FL=$%02x", cpu.FL);
        }

    TEST_END;

    TEST_BEGIN("ROT");
        LOAD(
            0x07, // RLC
            0x17, // RAL
            0x0f, // RRC
            0x1f, // RAR
        );

        cpu.A = 0x55;
        cpu_step(&cpu);
        if (cpu.A != 0xaa) {
            TEST_FAIL("RLC A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x02) {
            TEST_FAIL("RLC FL=$%02x", cpu.FL);
        }

        cpu_step(&cpu);
        if (cpu.A != 0x54) {
            TEST_FAIL("RAL A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x03) {
            TEST_FAIL("RAL FL=$%02x", cpu.FL);
        }

        cpu_step(&cpu);
        if (cpu.A != 0x2a) {
            TEST_FAIL("RRC A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x02) {
            TEST_FAIL("RRC FL=$%02x", cpu.FL);
        }

        cpu_step(&cpu);
        if (cpu.A != 0x15) {
            TEST_FAIL("RAR A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x02) {
            TEST_FAIL("RAR FL=$%02x", cpu.FL);
        }
    TEST_END;

    TEST_BEGIN("JMP");
        /* Testing true condition */
        LOAD_AT(0x0000, 0xc3, 0x50, 0x00); // JMP $0050
        LOAD_AT(0x0050, 0xc2, 0x00, 0x10); // JNZ $1000
        LOAD_AT(0x1000, 0xd2, 0x50, 0x10); // JNC $1050
        LOAD_AT(0x1050, 0xe2, 0x00, 0x20); // JPO $2000
        LOAD_AT(0x2000, 0xf2, 0x50, 0x20); // JP  $2050
        LOAD_AT(0x2050, 0xca, 0x00, 0x30); // JZ  $3000
        LOAD_AT(0x3000, 0xda, 0x50, 0x30); // JC  $3050
        LOAD_AT(0x3050, 0xea, 0x00, 0x40); // JPE $4000
        LOAD_AT(0x4000, 0xfa, 0x50, 0x40); // JM  $4050

        /* Testing false condition */
        LOAD_AT(0x4050,
            0xc2, 0x00, 0x50, // JNZ $5000
            0xd2, 0x50, 0x50, // JNC $5050
            0xe2, 0x00, 0x60, // JPO $6000
            0xf2, 0x50, 0x60, // JP  $6050
            0xca, 0x00, 0x70, // JZ  $7000
            0xda, 0x50, 0x70, // JC  $7050
            0xea, 0x00, 0x80, // JPE $8000
            0xfa, 0x50, 0x80, // JM  $8050
        );

        /* True condition */

        cpu.FL = 0;
        cpu_step(&cpu);
        if (cpu.PC != 0x0050) {
            TEST_FAIL("JMP PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 6) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x1000) {
            TEST_FAIL("JZ PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 0) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x1050) {
            TEST_FAIL("JC PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 2) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x2000) {
            TEST_FAIL("JPO PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 7) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x2050) {
            TEST_FAIL("JP PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 6) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x3000) {
            TEST_FAIL("JZ PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 0) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x3050) {
            TEST_FAIL("JC PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 2) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4000) {
            TEST_FAIL("JPE PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 7) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4050) {
            TEST_FAIL("JM PC=$%04x", cpu.PC);
        }


        /* False condition */

        cpu.FL = (1U << 6) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4053) {
            TEST_FAIL("JNZ' PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 0) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4056) {
            TEST_FAIL("JNC' PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 2) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4059) {
            TEST_FAIL("JPO' PC=$%04x", cpu.PC);
        }

        cpu.FL = (1U << 7) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x405c) {
            TEST_FAIL("JP' PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 6) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x405f) {
            TEST_FAIL("JZ' PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 0) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4062) {
            TEST_FAIL("JC' PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 2) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4065) {
            TEST_FAIL("JPE' PC=$%04x", cpu.PC);
        }

        cpu.FL = ~(1U << 7) & 0xff;
        cpu_step(&cpu);
        if (cpu.PC != 0x4068) {
            TEST_FAIL("JM' PC=$%04x", cpu.PC);
        }

    TEST_END;

    TEST_BEGIN("INX");
        LOAD(
            0x03, // INX B
            0x13, // INX D
            0x23, // INX H
            0x33, // INX SP
        );

        cpu.B = 0x0a; cpu.C = 0xff;
        cpu_step(&cpu);
        if (cpu.B != 0x0b) {
            TEST_FAIL("B=$%02x", cpu.B);
        }
        if (cpu.C != 0x00) {
            TEST_FAIL("C=$%02x", cpu.C);
        }

        cpu.D = 0xff; cpu.E = 0xff;
        cpu_step(&cpu);
        if (cpu.D != 0x00) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.E != 0x00) {
            TEST_FAIL("E=$%02x", cpu.E);
        }

        cpu.H = 0x0c; cpu.L = 0xfe;
        cpu_step(&cpu);
        if (cpu.H != 0x0c) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xff) {
            TEST_FAIL("L=$%02x", cpu.L);
        }

        cpu.SP = 0x00ff;
        cpu_step(&cpu);
        if (cpu.SP != 0x0100) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("DCX");
        LOAD(
            0x0b, // DCX B
            0x1b, // DCX D
            0x2b, // DCX H
            0x3b, // DCX SP
        );

        cpu.B = 0x01; cpu.C = 0x00;
        cpu_step(&cpu);
        if (cpu.B != 0x00) {
            TEST_FAIL("B=$%02x", cpu.B);
        }
        if (cpu.C != 0xff) {
            TEST_FAIL("B=$%02x", cpu.C);
        }

        cpu.D = 0x00; cpu.E = 0x00;
        cpu_step(&cpu);
        if (cpu.D != 0xff) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.E != 0xff) {
            TEST_FAIL("E=$%02x", cpu.E);
        }

        cpu.H = 0x0c; cpu.L = 0xfe;
        cpu_step(&cpu);
        if (cpu.H != 0x0c) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xfd) {
            TEST_FAIL("L=$%02x", cpu.L);
        }

        cpu.SP = 0x0100;
        cpu_step(&cpu);
        if (cpu.SP != 0x00ff) {
            TEST_FAIL("SP=$%04x", cpu.SP);
        }

    TEST_END;

    TEST_BEGIN("DAD");
        LOAD(
            0x09, // DAD B
            0x19, // DAD D
            0x29, // DAD H
            0x39, // DAD SP
        );

        cpu.H = 0x00; cpu.L = 0x00;

        cpu.B = 0xab; cpu.C = 0xcd;
        cpu_step(&cpu);
        if (cpu.H != 0xab) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.L != 0xcd) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.L != 0xcd) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.FL & 0x01) {
            TEST_FAIL("C=%d", cpu.FL & 0x01);
        }

        cpu.D = 0x11; cpu.E = 0x22;
        cpu_step(&cpu);
        if (cpu.H != 0xbc) {
            TEST_FAIL("H'=$%02x", cpu.H);
        }
        if (cpu.L != 0xef) {
            TEST_FAIL("L'=$%02x", cpu.L);
        }
        if (cpu.FL & 0x01) {
            TEST_FAIL("C'=%d", cpu.FL & 0x01);
        }

        // Shift HL by one
        cpu_step(&cpu);
        if (cpu.H != 0x79) {
            TEST_FAIL("H''=$%02x", cpu.H);
        }
        if (cpu.L != 0xde) {
            TEST_FAIL("L''=$%02x", cpu.L);
        }
        if (!(cpu.FL & 0x01)) {
            TEST_FAIL("C''=%d", cpu.FL & 0x01);
        }

        cpu.SP = 0x1fff;
        cpu_step(&cpu);
        if (cpu.H != 0x99) {
            TEST_FAIL("H'''=$%02x", cpu.H);
        }
        if (cpu.L != 0xdd) {
            TEST_FAIL("L'''=$%02x", cpu.L);
        }
        if (cpu.FL & 0x01) {
            TEST_FAIL("C'''=%d", cpu.FL & 0x01);
        }

    TEST_END;

    TEST_BEGIN("INR");
        LOAD(
            0x04, // INR B
            0x0c, // INR C
            0x14, // INR D
            0x1c, // INR E
            0x24, // INR H
            0x2c, // INR L
            0x34, // INR M
            0x3c, // INR A
        );

        cpu.B = 0x00;
        cpu_step(&cpu);
        if (cpu.B != 0x01) {
            TEST_FAIL("B=$%02x", cpu.B);
        }
        if (cpu.FL != 0x02) {
            TEST_FAIL("INR B FL=$%02x", cpu.FL);
        }

        cpu.C = 0xff;
        cpu_step(&cpu);
        if (cpu.C != 0x00) {
            TEST_FAIL("C=$%02x", cpu.C);
        }
        if (cpu.FL != 0x56) {
            TEST_FAIL("INR C FL=$%02x", cpu.FL);
        }

        cpu.D = 0x1f;
        cpu_step(&cpu);
        if (cpu.D != 0x20) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.FL != 0x12) {
            TEST_FAIL("INR D FL=$%02x", cpu.FL);
        }

        cpu.E = 0x80;
        cpu_step(&cpu);
        if (cpu.E != 0x81) {
            TEST_FAIL("E=$%02x", cpu.E);
        }
        if (cpu.FL != 0x86) {
            TEST_FAIL("INR E FL=$%02x", cpu.FL);
        }

        cpu.H = 0x7f;
        cpu_step(&cpu);
        if (cpu.H != 0x80) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.FL != 0x92) {
            TEST_FAIL("INR H FL=$%02x", cpu.FL);
        }

        cpu.L = 0x0e;
        cpu_step(&cpu);
        if (cpu.L != 0x0f) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.FL != 0x06) {
            TEST_FAIL("INR L FL=$%02x", cpu.FL);
        }

        ram[0x4000] = 0x10;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        if (ram[0x4000] != 0x11) {
            TEST_FAIL("M=$%02x", ram[0x4000]);
        }
        if (cpu.FL != 0x06) {
            TEST_FAIL("INR M FL=$%02x", cpu.FL);
        }

        cpu.A = 0x0f;
        cpu_step(&cpu);
        if (cpu.A != 0x10) {
            TEST_FAIL("A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x12) {
            TEST_FAIL("INR A FL=$%02x", cpu.FL);
        }

    TEST_END;

    TEST_BEGIN("DCR");
        LOAD(
            0x05, // DCR B
            0x0d, // DCR C
            0x15, // DCR D
            0x1d, // DCR E
            0x25, // DCR H
            0x2d, // DCR L
            0x35, // DCR M
            0x3d, // DCR A
        );

        cpu.B = 0x00;
        cpu_step(&cpu);
        if (cpu.B != 0xff) {
            TEST_FAIL("B=$%02x", cpu.B);
        }
        if (cpu.FL != 0x86) {
            TEST_FAIL("DCR B FL=$%02x", cpu.FL);
        }

        cpu.C = 0xff;
        cpu_step(&cpu);
        if (cpu.C != 0xfe) {
            TEST_FAIL("C=$%02x", cpu.C);
        }
        if (cpu.FL != 0x92) {
            TEST_FAIL("DCR C FL=$%02x", cpu.FL);
        }

        cpu.D = 0x1f;
        cpu_step(&cpu);
        if (cpu.D != 0x1e) {
            TEST_FAIL("D=$%02x", cpu.D);
        }
        if (cpu.FL != 0x16) {
            TEST_FAIL("DCR D FL=$%02x", cpu.FL);
        }

        cpu.E = 0x80;
        cpu_step(&cpu);
        if (cpu.E != 0x7f) {
            TEST_FAIL("E=$%02x", cpu.E);
        }
        if (cpu.FL != 0x02) {
            TEST_FAIL("DCR E FL=$%02x", cpu.FL);
        }

        cpu.H = 0x7f;
        cpu_step(&cpu);
        if (cpu.H != 0x7e) {
            TEST_FAIL("H=$%02x", cpu.H);
        }
        if (cpu.FL != 0x16) {
            TEST_FAIL("DCR H FL=$%02x", cpu.FL);
        }

        cpu.L = 0x0e;
        cpu_step(&cpu);
        if (cpu.L != 0x0d) {
            TEST_FAIL("L=$%02x", cpu.L);
        }
        if (cpu.FL != 0x12) {
            TEST_FAIL("DCR L FL=$%02x", cpu.FL);
        }

        ram[0x4000] = 0x10;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        if (ram[0x4000] != 0x0f) {
            TEST_FAIL("M=$%02x", ram[0x4000]);
        }
        if (cpu.FL != 0x06) {
            TEST_FAIL("DCR M FL=$%02x", cpu.FL);
        }

        cpu.A = 0x0f;
        cpu_step(&cpu);
        if (cpu.A != 0x0e) {
            TEST_FAIL("A=$%02x", cpu.A);
        }
        if (cpu.FL != 0x12) {
            TEST_FAIL("DCR A FL=$%02x", cpu.FL);
        }

    TEST_END;

    TEST_BEGIN("AND"); // TODO: test flags
        LOAD(
            0xa0,       // ANA B
            0xa1,       // ANA C
            0xa2,       // ANA D
            0xa3,       // ANA E
            0xa4,       // ANA H
            0xa5,       // ANA L
            0xa6,       // ANA M
            0xa7,       // ANA A
            0xe6, 0x5a, // ANI 0x5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("ANA B A=$%02x", cpu.A);
        }

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("ANA C A=$%02x", cpu.A);
        }

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("ANA D A=$%02x", cpu.A);
        }

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        if (cpu.A != 0x0f) {
            TEST_FAIL("ANA E A=$%02x", cpu.A);
        }

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        if (cpu.A != 0xf0) {
            TEST_FAIL("ANA H A=$%02x", cpu.A);
        }

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        if (cpu.A != 0x32) {
            TEST_FAIL("ANA L A=$%02x", cpu.A);
        }

        cpu.A = 0xaa; 
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        if (cpu.A != 0x88) {
            TEST_FAIL("ANA M A=$%02x", cpu.A);
        }

        cpu.A = 0xcc;
        cpu_step(&cpu);
        if (cpu.A != 0xcc) {
            TEST_FAIL("ANA A A=$%02x", cpu.A);
        }

        cpu.A = 0x41;
        cpu_step(&cpu);
        if (cpu.A != 0x40) {
            TEST_FAIL("ANI A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("OR"); // TODO: test flags
        LOAD(
            0xb0,       // ORA B
            0xb1,       // ORA C
            0xb2,       // ORA D
            0xb3,       // ORA E
            0xb4,       // ORA H
            0xb5,       // ORA L
            0xb6,       // ORA M
            0xb7,       // ORA A
            0xf6, 0x5a, // ORI 0x5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("ORA B A=$%02x", cpu.A);
        }

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x10) {
            TEST_FAIL("ORA C A=$%02x", cpu.A);
        }

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        if (cpu.A != 0xff) {
            TEST_FAIL("ORA D A=$%02x", cpu.A);
        }

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        if (cpu.A != 0xff) {
            TEST_FAIL("ORA E A=$%02x", cpu.A);
        }

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        if (cpu.A != 0xf1) {
            TEST_FAIL("ORA H A=$%02x", cpu.A);
        }

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        if (cpu.A != 0xf3) {
            TEST_FAIL("ORA L A=$%02x", cpu.A);
        }

        cpu.A = 0xaa; 
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        if (cpu.A != 0xff) {
            TEST_FAIL("ORA M A=$%02x", cpu.A);
        }

        cpu.A = 0xcc;
        cpu_step(&cpu);
        if (cpu.A != 0xcc) {
            TEST_FAIL("ORA A A=$%02x", cpu.A);
        }

        cpu.A = 0x41;
        cpu_step(&cpu);
        if (cpu.A != 0x5b) {
            TEST_FAIL("ORI A=$%02x", cpu.A);
        }

    TEST_END;

    TEST_BEGIN("XOR"); // TODO: test flags
        LOAD(
            0xa8,       // XRA B
            0xa9,       // XRA C
            0xaa,       // XRA D
            0xab,       // XRA E
            0xac,       // XRA H
            0xad,       // XRA L
            0xae,       // XRA M
            0xaf,       // XRA A
            0xee, 0x5a, // XRI 0x5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("XRA B A=$%02x", cpu.A);
        }

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        if (cpu.A != 0x10) {
            TEST_FAIL("XRA C A=$%02x", cpu.A);
        }

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        if (cpu.A != 0xff) {
            TEST_FAIL("XRA D A=$%02x", cpu.A);
        }

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        if (cpu.A != 0xf0) {
            TEST_FAIL("XRA E A=$%02x", cpu.A);
        }

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        if (cpu.A != 0x01) {
            TEST_FAIL("XRA H A=$%02x", cpu.A);
        }

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        if (cpu.A != 0xc1) {
            TEST_FAIL("XRA L A=$%02x", cpu.A);
        }

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        if (cpu.A != 0x77) {
            TEST_FAIL("XRA M A=$%02x", cpu.A);
        }

        cpu.A = 0xcc;
        cpu_step(&cpu);
        if (cpu.A != 0x00) {
            TEST_FAIL("ORA A A=$%02x", cpu.A);
        }

        cpu.A = 0x41;
        cpu_step(&cpu);
        if (cpu.A != 0x1b) {
            TEST_FAIL("ORI A=$%02x", cpu.A);
        }

    TEST_END;


}
