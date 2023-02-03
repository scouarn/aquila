#include <stdio.h>
#include <string.h>

#include "cpu.h"

#define RAM_SIZE 0x10000

#define TEST_BEGIN(X) do {                  \
    printf("Testing %-10s", X);             \
    fflush(stdout);                         \
    cpu_reset = true;                       \
    cpu_step();                             \
    memset(io_ram, 0, RAM_SIZE);

#define TEST_END \
    printf("\033[32mOK\033[0m\n"); \
} while(0)

#define TEST_FAIL(REASON, ...) \
    printf("\033[31mFAIL\033[0m: " REASON " at line %d\n", __VA_ARGS__ __VA_OPT__(,) __LINE__); \
    break

#define TEST_ASSERT_EQ(REG, VAL) \
    if ((REG) != (VAL)) { TEST_FAIL(#REG " is %d instead of %d", REG, VAL); }

#define TEST_ASSERT_EQ8(REG, VAL) \
    if ((REG) != (VAL)) { TEST_FAIL(#REG " is $%02x instead of $%02x", REG, VAL); }

#define TEST_ASSERT_EQ16(REG, VAL) \
    if ((REG) != (VAL)) { TEST_FAIL(#REG " is $%04x instead of $%04x", REG, VAL); }



#define RAM_LOAD_ARR(OFF, ARR) do {    \
    memcpy(io_ram+OFF, ARR, sizeof(ARR));      \
} while (0)

#define RAM_LOAD(OFF, ...) do {        \
    const data_t prog[] = { __VA_ARGS__ };  \
    RAM_LOAD_ARR(OFF, prog);           \
} while (0)

#define RAM_LOAD_FILE(OFF, FP) \
    fread(io_ram+OFF, 1, RAM_SIZE-OFF, FP);


data_t io_ram[RAM_SIZE];
data_t io_data_bus;
addr_t io_addr_bus;

data_t cpu_load(addr_t addr) {
    return io_ram[addr];
}

void cpu_store(addr_t addr, data_t data) {
    io_ram[addr] = data;
}

data_t cpu_input(port_t port) {
    return io_ram[port];
}

void cpu_output(port_t port, data_t data) {
    io_ram[port] = data;
}


int main(void) {

    TEST_BEGIN("MOV"); // TODO: more tests for these instructions
        RAM_LOAD(0x0000,
            0x7e, // MOV A, M
            0x47, // MOV B, A
            0x70, // MOV M, B
        );

        io_ram[0x0400] = 0xaa;
        cpu_HL = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xaa);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_B, 0xaa);

        cpu_HL = 0x0500;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0500], 0xaa);

    TEST_END;

    TEST_BEGIN("STAX");
        RAM_LOAD(0x0000,
            0x02, // STAX B
            0x12, // STAX D
        );

        cpu_A = 0xaa;
        cpu_BC = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0xaa);

        cpu_A = 0xbb;
        cpu_DE = 0x0500;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0500], 0xbb);

    TEST_END;

    TEST_BEGIN("STA");
        RAM_LOAD(0x0000,
            0x32, 0x00, 0x04, // STA $4000
        );

        cpu_A = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0xaa);
    TEST_END;

    TEST_BEGIN("LDAX");
        RAM_LOAD(0x0000,
            0x0a, // LDAX B
            0x1a, // LDAX D
        );

        io_ram[0x0400] = 0xaa;
        cpu_BC = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xaa);

        io_ram[0x0500] = 0xbb;
        cpu_DE = 0x0500;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xbb);
    TEST_END;

    TEST_BEGIN("LDA");
        RAM_LOAD(0x0000,
            0x3a, 0x00, 0x04, // LDA $4000
        );

        io_ram[0x0400] = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xaa);
    TEST_END;

    TEST_BEGIN("SHLD");
        RAM_LOAD(0x0000,
            0x22, 0x00, 0x04, // SHLD $4000
        );

        cpu_HL = 0xaabb;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0xbb);
        TEST_ASSERT_EQ8(io_ram[0x0401], 0xaa);
    TEST_END;

    TEST_BEGIN("LHLD");
        RAM_LOAD(0x0000,
            0x2a, 0x00, 0x04, // LHLD $4000
        );

        io_ram[0x0400] = 0xaa;
        io_ram[0x0401] = 0xbb;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0xbbaa);
    TEST_END;

    TEST_BEGIN("MVI");
        RAM_LOAD(0x0000,
            0x06, 0xbb, // MVI B, $bb
            0x0e, 0xcc, // MVI C, $cc
            0x16, 0xdd, // MVI D, $dd
            0x1e, 0xee, // MVI E, $ee
            0x26, 0xff, // MVI H, $ff
            0x2e, 0x11, // MVI L, $11
            0x36, 0x99, // MVI M, $99
            0x3e, 0xaa, // MVI A, $aa
        );

        cpu_step();
        TEST_ASSERT_EQ8(cpu_B, 0xbb);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_C, 0xcc);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_D, 0xdd);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_E, 0xee);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_H, 0xff);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_L, 0x11);

        cpu_HL = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0x99);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xaa);

    TEST_END;

    TEST_BEGIN("LXI");
        RAM_LOAD(0x0000,
            0x01, 0xcc, 0xbb, // LXI B, $bbcc
            0x11, 0xee, 0xdd, // LXI D, $ddee
            0x21, 0x11, 0xff, // LXI H, $ff11
            0x31, 0x44, 0x33, // LXI SP,$3344
        );

        cpu_step();
        TEST_ASSERT_EQ8(cpu_BC, 0xbbcc);

        cpu_step();
        TEST_ASSERT_EQ16(cpu_DE, 0xddee);

        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0xff11);

        cpu_step();
        TEST_ASSERT_EQ16(cpu_SP, 0x3344);

    TEST_END;

    TEST_BEGIN("XCHG");
        RAM_LOAD(0x0000,
            0xeb // XCHG
        );

        cpu_H = 0xaa; cpu_L = 0xbb;
        cpu_D = 0xdd; cpu_E = 0xee;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0xddee);
        TEST_ASSERT_EQ16(cpu_DE, 0xaabb);

    TEST_END;

    TEST_BEGIN("SPHL");
        RAM_LOAD(0x0000,
            0xf9 // SPHL
        );

        cpu_H = 0xaa; cpu_L = 0xbb;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_SP, 0xaabb);

    TEST_END;

    TEST_BEGIN("PUSH"); // TODO: test with other registers
        RAM_LOAD(0x0000,
            0xd5, // PUSH D
        );

        cpu_DE = 0x8f9d;
        cpu_SP = 0x0a2c;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0a2b], 0x8f);
        TEST_ASSERT_EQ8(io_ram[0x0a2a], 0x9d);
        TEST_ASSERT_EQ16(cpu_SP, 0x0a2a);

    TEST_END;

    TEST_BEGIN("POP"); // TODO: test with other registers
        RAM_LOAD(0x0000,
            0xe1, // POP H
        );

        io_ram[0x0239] = 0x3d;
        io_ram[0x023A] = 0x93;
        cpu_SP = 0x0239;

        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x933d);
        TEST_ASSERT_EQ16(cpu_SP, 0x023b);
    TEST_END;

    TEST_BEGIN("XTHL");
        RAM_LOAD(0x0000,
            0xe3, // XTHL
        );

        cpu_SP = 0x00ad;
        cpu_H = 0x0b; cpu_L = 0x3c;
        io_ram[0x00ad] = 0xf0;
        io_ram[0x00ae] = 0x0d;

        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x0df0);
        TEST_ASSERT_EQ8(io_ram[0x00ae], 0x0b);
        TEST_ASSERT_EQ8(io_ram[0x00ad], 0x3c);
        TEST_ASSERT_EQ16(cpu_SP, 0x00ad);

    TEST_END;

    TEST_BEGIN("EI-DI");
        RAM_LOAD(0x0000,
            0xfb, // EI
            0xf3, // DI
        );

        TEST_ASSERT_EQ(cpu_inte, false);

        cpu_step();
        TEST_ASSERT_EQ(cpu_inte, true);

        cpu_step();
        TEST_ASSERT_EQ(cpu_inte, false);

    TEST_END;

    TEST_BEGIN("STC-CMC");
        RAM_LOAD(0x0000,
            0x37, // STC
            0x3f, // CMC
        );

        TEST_ASSERT_EQ(cpu_FL & FLAG_C, 0);

        cpu_step();
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, FLAG_C);

        cpu_step();
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, 0);

    TEST_END;

    TEST_BEGIN("CMA");
        RAM_LOAD(0x0000,
            0x2f, // CMA
        );

        cpu_A = 0x55;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xaa);

    TEST_END;

    TEST_BEGIN("DAA");
        RAM_LOAD(0x0000,
            0x3e, 0x08, // MVI A, 08
            0xc6, 0x93, // ADI 93
            0x27,       // DAA
        );

        cpu_step();
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x9b);
        TEST_ASSERT_EQ8(cpu_FL, 0x82);
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x01);
        TEST_ASSERT_EQ8(cpu_FL, 0x13);

    TEST_END;

    TEST_BEGIN("ROT");
        RAM_LOAD(0x0000,
            0x07, // RLC
            0x17, // RAL
            0x0f, // RRC
            0x1f, // RAR
        );

        cpu_A = 0x55;
        cpu_FL = 2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0xaa);
        TEST_ASSERT_EQ8(cpu_FL, 0x02);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x54);
        TEST_ASSERT_EQ8(cpu_FL, 0x03);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x2a);
        TEST_ASSERT_EQ8(cpu_FL, 0x02);

        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x15);
        TEST_ASSERT_EQ8(cpu_FL, 0x02);

    TEST_END;

    TEST_BEGIN("JMP");
        /* Testing true condition */
        RAM_LOAD(0x0000, 0xc3, 0x50, 0x00); // JMP $0050
        RAM_LOAD(0x0050, 0xc2, 0x00, 0x01); // JNZ $0100
        RAM_LOAD(0x0100, 0xd2, 0x50, 0x01); // JNC $0150
        RAM_LOAD(0x0150, 0xe2, 0x00, 0x02); // JPO $0200
        RAM_LOAD(0x0200, 0xf2, 0x50, 0x02); // JP  $0250
        RAM_LOAD(0x0250, 0xca, 0x00, 0x03); // JZ  $0300
        RAM_LOAD(0x0300, 0xda, 0x50, 0x03); // JC  $0350
        RAM_LOAD(0x0350, 0xea, 0x00, 0x04); // JPE $0400
        RAM_LOAD(0x0400, 0xfa, 0x50, 0x04); // JM  $0450

        /* Testing false condition */
        RAM_LOAD(0x0450,
            0xc2, 0x00, 0x05, // JNZ $0500
            0xd2, 0x50, 0x05, // JNC $0550
            0xe2, 0x00, 0x06, // JPO $0600
            0xf2, 0x50, 0x06, // JP  $0650
            0xca, 0x00, 0x07, // JZ  $0700
            0xda, 0x50, 0x07, // JC  $0750
            0xea, 0x00, 0x08, // JPE $0800
            0xfa, 0x50, 0x08, // JM  $0850
        );

        /* True condition */

        cpu_FL = 0;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0050);

        cpu_FL = ~FLAG_Z;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0100);

        cpu_FL = ~FLAG_C;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0150);

        cpu_FL = ~FLAG_P;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0200);

        cpu_FL = (data_t)~FLAG_S;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0250);

        cpu_FL = FLAG_Z;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0300);

        cpu_FL = FLAG_C;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0350);

        cpu_FL = FLAG_P;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0400);

        cpu_FL = FLAG_S;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0450);


        /* False condition */

        cpu_FL = FLAG_Z;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0453);

        cpu_FL = FLAG_C;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0456);

        cpu_FL = FLAG_P;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0459);

        cpu_FL = FLAG_S;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x045c);

        cpu_FL = ~FLAG_Z;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x045f);

        cpu_FL = ~FLAG_C;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0462);

        cpu_FL = ~FLAG_P;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0465);

        cpu_FL = (data_t)~FLAG_S;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_PC, 0x0468);

    TEST_END;

    TEST_BEGIN("INX");
        RAM_LOAD(0x0000,
            0x03, // INX B
            0x13, // INX D
            0x23, // INX H
            0x33, // INX SP
        );

        cpu_B = 0x0a; cpu_C = 0xff;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_BC, 0x0b00);

        cpu_D = 0xff; cpu_E = 0xff;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_DE, 0x0000);

        cpu_H = 0x0c; cpu_L = 0xfe;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x0cff);

        cpu_SP = 0x00ff;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_SP, 0x0100);

    TEST_END;

    TEST_BEGIN("DCX");
        RAM_LOAD(0x0000,
            0x0b, // DCX B
            0x1b, // DCX D
            0x2b, // DCX H
            0x3b, // DCX SP
        );

        cpu_B = 0x01; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_BC, 0x00ff);

        cpu_D = 0x00; cpu_E = 0x00;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_DE, 0xffff);

        cpu_H = 0x0c; cpu_L = 0xfe;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x0cfd);

        cpu_SP = 0x0100;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_SP, 0x00ff);

    TEST_END;

    TEST_BEGIN("DAD");
        RAM_LOAD(0x0000,
            0x09, // DAD B
            0x19, // DAD D
            0x29, // DAD H
            0x39, // DAD SP
        );

        cpu_HL = 0x0000;
        cpu_BC = 0xabcd;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0xabcd);
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, 0);

        cpu_DE = 0x1122;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0xbcef);
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, 0);

        // Add HL to HL (shift HL by one)
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x79de);
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, FLAG_C);

        cpu_SP = 0x1fff;
        cpu_step();
        TEST_ASSERT_EQ16(cpu_HL, 0x99dd);
        TEST_ASSERT_EQ(cpu_FL & FLAG_C, 0);

    TEST_END;

    TEST_BEGIN("INR");
        RAM_LOAD(0x0000,
            0x04, // INR B
            0x0c, // INR C
            0x14, // INR D
            0x1c, // INR E
            0x24, // INR H
            0x2c, // INR L
            0x34, // INR M
            0x3c, // INR A
        );

        /* Reset carry */
        cpu_FL &= ~FLAG_C;

        cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_B, 0x01);
        TEST_ASSERT_EQ8(cpu_FL, 0x02);

        cpu_C = 0xff;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_C, 0x00);
        TEST_ASSERT_EQ8(cpu_FL, 0x56);

        cpu_D = 0x1f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_D, 0x20);
        TEST_ASSERT_EQ8(cpu_FL, 0x12);

        cpu_E = 0x80;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_E, 0x81);
        TEST_ASSERT_EQ8(cpu_FL, 0x86);

        cpu_H = 0x7f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_H, 0x80);
        TEST_ASSERT_EQ8(cpu_FL, 0x92);

        cpu_L = 0x0e;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_L, 0x0f);
        TEST_ASSERT_EQ8(cpu_FL, 0x06);

        io_ram[0x0400] = 0x10;
        cpu_HL = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0x11);
        TEST_ASSERT_EQ8(cpu_FL, 0x06);

        cpu_A = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);
        TEST_ASSERT_EQ8(cpu_FL, 0x12);

    TEST_END;

    TEST_BEGIN("DCR");
        RAM_LOAD(0x0000,
            0x05, // DCR B
            0x0d, // DCR C
            0x15, // DCR D
            0x1d, // DCR E
            0x25, // DCR H
            0x2d, // DCR L
            0x35, // DCR M
            0x3d, // DCR A
        );

        /* Reset carry */
        cpu_FL &= ~FLAG_C;

        cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_B, 0xff);
        TEST_ASSERT_EQ8(cpu_FL, 0x86);

        cpu_C = 0xff;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_C, 0xfe);
        TEST_ASSERT_EQ8(cpu_FL, 0x92);

        cpu_D = 0x1f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_D, 0x1e);
        TEST_ASSERT_EQ8(cpu_FL, 0x16);

        cpu_E = 0x80;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_E, 0x7f);
        TEST_ASSERT_EQ8(cpu_FL, 0x02);

        cpu_H = 0x7f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_H, 0x7e);
        TEST_ASSERT_EQ8(cpu_FL, 0x16);

        cpu_L = 0x0e;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_L, 0x0d);
        TEST_ASSERT_EQ8(cpu_FL, 0x12);

        io_ram[0x0400] = 0x10;
        cpu_HL = 0x0400;
        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x0400], 0x0f);
        TEST_ASSERT_EQ8(cpu_FL, 0x06);

        cpu_A = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x0e);
        TEST_ASSERT_EQ8(cpu_FL, 0x12);

    TEST_END;

    TEST_BEGIN("AND"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x0f);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xf0);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x32);

        cpu_A = 0xaa; 
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x88);

        cpu_A = 0xcc;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xcc);

        cpu_A = 0x41;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x40);

    TEST_END;

    TEST_BEGIN("OR"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xf1);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xf3);

        cpu_A = 0xaa; 
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0xcc;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xcc);

        cpu_A = 0x41;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x5b);

    TEST_END;

    TEST_BEGIN("XOR"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xf0);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x01);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xc1);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x77);

        cpu_A = 0xcc;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x41;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x1b);

    TEST_END;

    TEST_BEGIN("ADD"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x0e);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xe1);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x25);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x87);

        cpu_A = 0xcc;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x98);

        cpu_A = 0x41;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x9b);

    TEST_END;

    TEST_BEGIN("SUB"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xab);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xf0);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x41);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xcd);

        cpu_A = 0xcc;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x41;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xe7);

    TEST_END;

    TEST_BEGIN("ADC"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x01);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x0e);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xe2);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x25);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x88);

        cpu_A = 0xcc;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x98);

        cpu_A = 0x41;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x9c);

    TEST_END;

    TEST_BEGIN("SBB"); // TODO: test flags
        RAM_LOAD(0x0000,
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

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x00);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x0f);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xab);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xef);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x40);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xcc);

        cpu_A = 0xcc;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xff);

        cpu_A = 0x41;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0xe6);

    TEST_END;

    TEST_BEGIN("CMP");
        RAM_LOAD(0x0000,
            0xb8,       // CMP B
            0xb9,       // CMP C
            0xba,       // CMP D
            0xbb,       // CMP E
            0xbc,       // CMP H
            0xbd,       // CMP L
            0xbe,       // CMP M
            0xbf,       // CMP A
            0xfe, 0x5a, // CPI $5a
        );

        cpu_A = 0x00; cpu_B = 0x00;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x00);
        TEST_ASSERT_EQ8(cpu_FL, 0x56);

        cpu_A = 0x10; cpu_C = 0x00;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x10);
        TEST_ASSERT_EQ8(cpu_FL, 0x12);

        cpu_A = 0x55; cpu_D = 0xaa;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x55);
        TEST_ASSERT_EQ8(cpu_FL, 0x83);

        cpu_A = 0xff; cpu_E = 0x0f;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0xff);
        TEST_ASSERT_EQ8(cpu_FL, 0x96);

        cpu_A = 0xf0; cpu_H = 0xf1;
        cpu_FL = 0x02;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0xf0);
        TEST_ASSERT_EQ8(cpu_FL, 0x87);

        cpu_A = 0x33; cpu_L = 0xf2;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x33);
        TEST_ASSERT_EQ8(cpu_FL, 0x17);

        cpu_A = 0xaa;
        cpu_HL = 0x0400;
        io_ram[0x0400] = 0xdd;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0xaa);
        TEST_ASSERT_EQ8(cpu_FL, 0x83);

        cpu_A = 0xcc;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0xcc);
        TEST_ASSERT_EQ8(cpu_FL, 0x56);

        cpu_A = 0x41;
        cpu_FL = 0x03;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A,  0x41);
        TEST_ASSERT_EQ8(cpu_FL, 0x87);

    TEST_END;

    TEST_BEGIN("IN-OUT");
        RAM_LOAD(0x0000,
            0xdb, 0x80, // IN  $80
            0xd3, 0x81, // OUT $81
        );

        io_ram[0x80] = 0x99;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x99);

        cpu_step();
        TEST_ASSERT_EQ8(io_ram[0x81], 0x99);

    TEST_END;

    // TODO tests for PCHL RET CALL RST
    TEST_BEGIN("IRQ");
        data_t ins[3];

        /* RST 4 procedure */
        RAM_LOAD(0x0020,
            0x3e, 0x77, // MVI A, $77
            0xfb,       // EI
            0xc9,       // RET
        );

        /* Main program */
        RAM_LOAD(0x0100,
            0x3e, 0x10, // MVI A, $10
            0x3e, 0x11, // MVI A, $11
            0x3e, 0x12, // MVI A, $12
            0x3e, 0x13, // MVI A, $13
            0x3e, 0x14, // MVI A, $14
        );

        /* Step normal instruction */
        cpu_PC = 0x0100;
        cpu_inte = true;
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x10);
        TEST_ASSERT_EQ16(cpu_PC, 0x102);

        /* Send IRQ with instruction */
        ins[0] = 0x3e; // MVI A, $99
        ins[1] = 0x99;
        cpu_irq_data = ins;
        cpu_step();
        TEST_ASSERT_EQ(cpu_irq_ack, 1);
        TEST_ASSERT_EQ8(cpu_A, 0x99);
        TEST_ASSERT_EQ16(cpu_PC, 0x102); // Normal execution continues

        /* Resend IRQ (not executed because INTE not reenabled) */
        cpu_irq_data = ins;
        cpu_step(); // MVI A, $11 is executed
        TEST_ASSERT_EQ(cpu_irq_ack, 0);
        TEST_ASSERT_EQ8(cpu_A, 0x11);
        TEST_ASSERT_EQ16(cpu_PC, 0x104);

        /* Step after IRQ (continue execution) */
        cpu_step();
        TEST_ASSERT_EQ8(cpu_A, 0x12);
        TEST_ASSERT_EQ16(cpu_PC, 0x106);

        /* Reset inte, send another IRQ */
        ins[0] = 0x3a; // LDA $0400
        ins[1] = 0x00;
        ins[2] = 0x04;
        io_ram[0x0400] = 0x88;
        cpu_inte = true;
        cpu_irq_data = ins;
        cpu_step();
        TEST_ASSERT_EQ(cpu_inte, false);
        TEST_ASSERT_EQ(cpu_irq_ack, 1);
        TEST_ASSERT_EQ8(cpu_A, 0x88);
        TEST_ASSERT_EQ16(cpu_PC, 0x106);

        /* Resend again (fail, interrupts still disabled) */
        cpu_irq_data = ins;
        cpu_step(); // MVI A, $13 is executed instead
        TEST_ASSERT_EQ(cpu_inte, false);
        TEST_ASSERT_EQ(cpu_irq_ack, 0);
        TEST_ASSERT_EQ8(cpu_A, 0x13);
        TEST_ASSERT_EQ16(cpu_PC, 0x108);

        /* Reset inte, send RST instruction */
        ins[0] = 0xe7; // RST 4
        cpu_inte = true;
        cpu_irq_data = ins;
        cpu_step();
        TEST_ASSERT_EQ(cpu_irq_ack, 1);
        TEST_ASSERT_EQ8(cpu_A, 0x13);
        TEST_ASSERT_EQ16(cpu_PC, 0x20);

        /* Resend (fail) */
        ins[0] = 0xe7; // RST 4
        cpu_irq_data = ins;
        cpu_step(); // MVI A, $77 is executed
        TEST_ASSERT_EQ(cpu_inte, false);
        TEST_ASSERT_EQ(cpu_irq_ack, false);
        TEST_ASSERT_EQ(cpu_irq_data == NULL, true);
        TEST_ASSERT_EQ8(cpu_A, 0x77);
        TEST_ASSERT_EQ16(cpu_PC, 0x22);

        /* Step through RST 4 procedure */
        cpu_step(); // EI
        TEST_ASSERT_EQ(cpu_inte, true);
        TEST_ASSERT_EQ(cpu_irq_data == NULL, true);
        TEST_ASSERT_EQ8(cpu_A, 0x77);
        TEST_ASSERT_EQ16(cpu_PC, 0x23);

        cpu_step(); // RET
        TEST_ASSERT_EQ(cpu_inte, true);
        TEST_ASSERT_EQ8(cpu_A, 0x77);
        TEST_ASSERT_EQ16(cpu_PC, 0x108);

        cpu_step(); // MVI A, $12 (back to main program)
        TEST_ASSERT_EQ(cpu_inte, true);
        TEST_ASSERT_EQ8(cpu_A, 0x14);
        TEST_ASSERT_EQ16(cpu_PC, 0x10a);

    TEST_END;
}
