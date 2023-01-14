#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "ram.h"

#define LOAD_AT(OFF, ...) do {              \
    data_t prog[] = { __VA_ARGS__ };        \
    memcpy(ram+OFF, prog, sizeof(prog));    \
} while (0)

#define LOAD(...) LOAD_AT(0, __VA_ARGS__)

#define TEST_BEGIN(X) do {                  \
    printf("Testing %-10s", X);             \
    fflush(stdout);                         \
    cpu_reset(&cpu);                        \
    memset(ram, 0, RAM_SIZE);

#define TEST_END \
    printf("\033[32mOK\033[0m\n"); \
} while(0)

#define TEST_FAIL(REASON, ...) \
    printf("\033[31mFAIL\033[0m: " REASON " at line %d\n", __VA_ARGS__ __VA_OPT__(,) __LINE__); \
    cpu_dump(&cpu, stdout); \
    break

#define TEST_ASSERT_EQ(REG, VAL) \
    if (REG != VAL) { TEST_FAIL(#REG " is $%02x instead of $%02x", REG, VAL); }


static cpu_t cpu;
static data_t ram[RAM_SIZE];

static data_t load(addr_t addr) {
    return ram[addr];
}

static void store(addr_t addr, data_t data) {
    ram[addr] = data;
}

static data_t input(port_t port) {
    return ram[port];
}

static void output(port_t port, data_t data) {
    ram[port] = data;
}


int main(void) {

    /* Init CPU */
    cpu.load   = load;
    cpu.store  = store;
    cpu.input  = input;
    cpu.output = output;

    TEST_BEGIN("MOV"); // TODO: more tests for these instructions
        LOAD(
            0x7e, // MOV A, M
            0x47, // MOV B, A
            0x70, // MOV M, B
        );

        ram[0x4000] = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xaa);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.B, 0xaa);

        cpu.H = 0x50; cpu.L = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x5000], 0xaa);

    TEST_END;

    TEST_BEGIN("STAX");
        LOAD(
            0x02, // STAX B
            0x12, // STAX D
        );

        cpu.A = 0xaa;
        cpu.B = 0x40; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0xaa);

        cpu.A = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x5000], 0xbb);

    TEST_END;

    TEST_BEGIN("STA");
        LOAD(
            0x32, 0x00, 0x40, // STA $4000
        );

        cpu.A = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0xaa);
    TEST_END;

    TEST_BEGIN("LDAX");
        LOAD(
            0x0a, // LDAX B
            0x1a, // LDAX D
        );

        ram[0x4000] = 0xaa;
        cpu.B = 0x40; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xaa);

        ram[0x5000] = 0xbb;
        cpu.D = 0x50; cpu.E = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xbb);

    TEST_END;

    TEST_BEGIN("LDA");
        LOAD(
            0x3a, 0x00, 0x40, // LDA $4000
        );

        ram[0x4000] = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xaa);
    TEST_END;

    TEST_BEGIN("SHLD");
        LOAD(
            0x22, 0x00, 0x40, // SHLD $4000
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0xbb);
        TEST_ASSERT_EQ(ram[0x4001], 0xaa);

    TEST_END;

    TEST_BEGIN("LHLD");
        LOAD(
            0x2a, 0x00, 0x40, // LHLD $4000
        );

        ram[0x4000] = 0xaa;
        ram[0x4001] = 0xbb;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0xbb);
        TEST_ASSERT_EQ(cpu.L, 0xaa);

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
        TEST_ASSERT_EQ(cpu.B, 0xbb);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.C, 0xcc);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0xdd);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.E, 0xee);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0xff);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.L, 0x11);

        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0x99);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xaa);

    TEST_END;

    TEST_BEGIN("LXI");
        LOAD(
            0x01, 0xcc, 0xbb, // LXI B, $bbcc
            0x11, 0xee, 0xdd, // LXI D, $ddee
            0x21, 0x11, 0xff, // LXI H, $ff11
            0x31, 0x44, 0x33, // LXI SP,$3344
        );

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.B, 0xbb);
        TEST_ASSERT_EQ(cpu.C, 0xcc);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0xdd);
        TEST_ASSERT_EQ(cpu.E, 0xee);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0xff);
        TEST_ASSERT_EQ(cpu.L, 0x11);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.SP, 0x3344);

    TEST_END;

    TEST_BEGIN("XCHG");
        LOAD(
            0xeb // XCHG
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu.D = 0xdd; cpu.E = 0xee;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0xdd);
        TEST_ASSERT_EQ(cpu.L, 0xee);
        TEST_ASSERT_EQ(cpu.D, 0xaa);
        TEST_ASSERT_EQ(cpu.E, 0xbb);

    TEST_END;

    TEST_BEGIN("SPHL");
        LOAD(
            0xf9 // SPHL
        );

        cpu.H = 0xaa; cpu.L = 0xbb;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.SP, 0xaabb);

    TEST_END;

    TEST_BEGIN("PUSH"); // TODO: test with other registers
        LOAD(
            0xd5, // PUSH D
        );

        cpu.D = 0x8f; cpu.E = 0x9d;
        cpu.SP = 0x3a2c;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x3a2b], 0x8f);
        TEST_ASSERT_EQ(ram[0x3a2a], 0x9d);
        TEST_ASSERT_EQ(cpu.SP, 0x3a2a);

    TEST_END;

    TEST_BEGIN("POP"); // TODO: test with other registers
        LOAD(
            0xe1, // POP H
        );

        ram[0x1239] = 0x3d;
        ram[0x123A] = 0x93;
        cpu.SP = 0x1239;

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x93);
        TEST_ASSERT_EQ(cpu.L, 0x3D);
        TEST_ASSERT_EQ(cpu.SP, 0x123b);

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
        TEST_ASSERT_EQ(cpu.H, 0x0d);
        TEST_ASSERT_EQ(cpu.L, 0xf0);
        TEST_ASSERT_EQ(ram[0x10ae], 0x0b);
        TEST_ASSERT_EQ(ram[0x10ad], 0x3c);
        TEST_ASSERT_EQ(cpu.SP, 0x10ad);

    TEST_END;

    TEST_BEGIN("EI-DI");
        LOAD(
            0xfb, // EI
            0xf3, // DI
        );

        TEST_ASSERT_EQ(cpu.inte, false);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.inte, true);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.inte, false);

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
        TEST_ASSERT_EQ(cpu.FL, 0x03);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

    TEST_END;

    TEST_BEGIN("CMA");
        LOAD(
            0x2f, // CMA
        );

        cpu.A = 0x55;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xaa);

    TEST_END;

    TEST_BEGIN("DAA");
        LOAD(
            0x27, // DAA
        );

        cpu.A = 0x9b;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x01);
        TEST_ASSERT_EQ(cpu.FL, 0x13);

    TEST_END;

    TEST_BEGIN("ROT");
        LOAD(
            0x07, // RLC
            0x17, // RAL
            0x0f, // RRC
            0x1f, // RAR
        );

        cpu.A = 0x55;
        cpu.FL = 2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A,  0xaa);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x54);
        TEST_ASSERT_EQ(cpu.FL, 0x03);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x2a);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x15);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

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
        TEST_ASSERT_EQ(cpu.PC, 0x0050);

        cpu.FL = ~(1U << 6) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x1000);

        cpu.FL = ~(1U << 0) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x1050);

        cpu.FL = ~(1U << 2) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x2000);

        cpu.FL = ~(1U << 7) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x2050);

        cpu.FL = (1U << 6) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x3000);

        cpu.FL = (1U << 0) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x3050);

        cpu.FL = (1U << 2) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4000);

        cpu.FL = (1U << 7) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4050);


        /* False condition */

        cpu.FL = (1U << 6) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4053);

        cpu.FL = (1U << 0) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4056);

        cpu.FL = (1U << 2) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4059);

        cpu.FL = (1U << 7) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x405c);

        cpu.FL = ~(1U << 6) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x405f);

        cpu.FL = ~(1U << 0) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4062);

        cpu.FL = ~(1U << 2) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4065);

        cpu.FL = ~(1U << 7) & 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.PC, 0x4068);

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
        TEST_ASSERT_EQ(cpu.B, 0x0b);
        TEST_ASSERT_EQ(cpu.C, 0x00);

        cpu.D = 0xff; cpu.E = 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0x00);
        TEST_ASSERT_EQ(cpu.E, 0x00);

        cpu.H = 0x0c; cpu.L = 0xfe;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x0c);
        TEST_ASSERT_EQ(cpu.L, 0xff);

        cpu.SP = 0x00ff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.SP, 0x0100);

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
        TEST_ASSERT_EQ(cpu.B, 0x00);
        TEST_ASSERT_EQ(cpu.C, 0xff);

        cpu.D = 0x00; cpu.E = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0xff);
        TEST_ASSERT_EQ(cpu.E, 0xff);

        cpu.H = 0x0c; cpu.L = 0xfe;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x0c);
        TEST_ASSERT_EQ(cpu.L, 0xfd);

        cpu.SP = 0x0100;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.SP, 0x00ff);

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
        TEST_ASSERT_EQ(cpu.H, 0xab);
        TEST_ASSERT_EQ(cpu.L, 0xcd);
        if (cpu.FL & 0x01) {
            TEST_FAIL("C=%d", cpu.FL & 0x01);
        }

        cpu.D = 0x11; cpu.E = 0x22;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0xbc);
        TEST_ASSERT_EQ(cpu.L, 0xef);
        if (cpu.FL & 0x01) {
            TEST_FAIL("C'=%d", cpu.FL & 0x01);
        }

        // Shift HL by one
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x79);
        TEST_ASSERT_EQ(cpu.L, 0xde);
        if (!(cpu.FL & 0x01)) {
            TEST_FAIL("C''=%d", cpu.FL & 0x01);
        }

        cpu.SP = 0x1fff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x99);
        TEST_ASSERT_EQ(cpu.L, 0xdd);
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
        TEST_ASSERT_EQ(cpu.B, 0x01);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

        cpu.C = 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.C, 0x00);
        TEST_ASSERT_EQ(cpu.FL, 0x56);

        cpu.D = 0x1f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0x20);
        TEST_ASSERT_EQ(cpu.FL, 0x12);

        cpu.E = 0x80;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.E, 0x81);
        TEST_ASSERT_EQ(cpu.FL, 0x86);

        cpu.H = 0x7f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x80);
        TEST_ASSERT_EQ(cpu.FL, 0x92);

        cpu.L = 0x0e;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.L, 0x0f);
        TEST_ASSERT_EQ(cpu.FL, 0x06);

        ram[0x4000] = 0x10;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0x11);
        TEST_ASSERT_EQ(cpu.FL, 0x06);

        cpu.A = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);
        TEST_ASSERT_EQ(cpu.FL, 0x12);

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
        TEST_ASSERT_EQ(cpu.B, 0xff);
        TEST_ASSERT_EQ(cpu.FL, 0x86);

        cpu.C = 0xff;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.C, 0xfe);
        TEST_ASSERT_EQ(cpu.FL, 0x92);

        cpu.D = 0x1f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.D, 0x1e);
        TEST_ASSERT_EQ(cpu.FL, 0x16);

        cpu.E = 0x80;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.E, 0x7f);
        TEST_ASSERT_EQ(cpu.FL, 0x02);

        cpu.H = 0x7f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.H, 0x7e);
        TEST_ASSERT_EQ(cpu.FL, 0x16);

        cpu.L = 0x0e;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.L, 0x0d);
        TEST_ASSERT_EQ(cpu.FL, 0x12);

        ram[0x4000] = 0x10;
        cpu.H = 0x40; cpu.L = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x4000], 0x0f);
        TEST_ASSERT_EQ(cpu.FL, 0x06);

        cpu.A = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0e);
        TEST_ASSERT_EQ(cpu.FL, 0x12);

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
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0f);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xf0);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x32);

        cpu.A = 0xaa; 
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x88);

        cpu.A = 0xcc;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xcc);

        cpu.A = 0x41;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x40);

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
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xf1);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xf3);

        cpu.A = 0xaa; 
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0xcc;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xcc);

        cpu.A = 0x41;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x5b);

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
            0xee, 0x5a, // XRI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xf0);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x01);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xc1);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x77);

        cpu.A = 0xcc;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x41;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x1b);

    TEST_END;

    TEST_BEGIN("ADD"); // TODO: test flags
        LOAD(
            0x80,       // ADD B
            0x81,       // ADD C
            0x82,       // ADD D
            0x83,       // ADD E
            0x84,       // ADD H
            0x85,       // ADD L
            0x86,       // ADD M
            0x87,       // ADD A
            0xc6, 0x5a, // ADI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0e);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xe1);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x25);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x87);

        cpu.A = 0xcc;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x98);

        cpu.A = 0x41;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x9b);

    TEST_END;

    TEST_BEGIN("SUB"); // TODO: test flags
        LOAD(
            0x90,       // SUB B
            0x91,       // SUB C
            0x92,       // SUB D
            0x93,       // SUB E
            0x94,       // SUB H
            0x95,       // SUB L
            0x96,       // SUB M
            0x97,       // SUB A
            0xd6, 0x5a, // SUI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xab);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xf0);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x41);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xcd);

        cpu.A = 0xcc;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x41;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xe7);

    TEST_END;

    TEST_BEGIN("ADC"); // TODO: test flags
        LOAD(
            0x88,       // ADC B
            0x89,       // ADC C
            0x8a,       // ADC D
            0x8b,       // ADC E
            0x8c,       // ADC H
            0x8d,       // ADC L
            0x8e,       // ADC M
            0x8f,       // ADC A
            0xce, 0x5a, // ACI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x01);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x10);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0e);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xe2);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x25);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x88);

        cpu.A = 0xcc;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x98);

        cpu.A = 0x41;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x9c);

    TEST_END;

    TEST_BEGIN("SBB"); // TODO: test flags
        LOAD(
            0x98,       // SBB B
            0x99,       // SBB C
            0x9a,       // SBB D
            0x9b,       // SBB E
            0x9c,       // SBB H
            0x9d,       // SBB L
            0x9e,       // SBB M
            0x9f,       // SBB A
            0xde, 0x5a, // SBI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0f);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xab);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xef);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x40);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xcc);

        cpu.A = 0xcc;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0x41;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xe6);

    TEST_END;

#if 0
    TEST_BEGIN("CMP");
        LOAD(
            0x98,       // SBB B
            0x99,       // SBB C
            0x9a,       // SBB D
            0x9b,       // SBB E
            0x9c,       // SBB H
            0x9d,       // SBB L
            0x9e,       // SBB M
            0x9f,       // SBB A
            0xde, 0x5a, // SBI $5a
        );

        cpu.A = 0x00; cpu.B = 0x00;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x00);

        cpu.A = 0x10; cpu.C = 0x00;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x0f);

        cpu.A = 0x55; cpu.D = 0xaa;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xab);

        cpu.A = 0xff; cpu.E = 0x0f;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xef);

        cpu.A = 0xf0; cpu.H = 0xf1;
        cpu.FL = 0x02;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0x33; cpu.L = 0xf2;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x40);

        cpu.A = 0xaa;
        cpu.H = 0x40; cpu.L = 0x00;
        ram[0x4000] = 0xdd;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xcc);

        cpu.A = 0xcc;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xff);

        cpu.A = 0x41;
        cpu.FL = 0x03;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0xe6);

    TEST_END;
#endif

    TEST_BEGIN("IN-OUT");
        LOAD(
            0xdb, 0x80, // IN  $80
            0xd3, 0x81, // OUT $81
        );

        ram[0x80] = 0x99;
        cpu_step(&cpu);
        TEST_ASSERT_EQ(cpu.A, 0x99);

        cpu_step(&cpu);
        TEST_ASSERT_EQ(ram[0x81], 0x99);

    TEST_END;

}
