#include <string.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "../emul/cpu.h"
#include "../emul/io.h"

#include "panel.h"

/* Boot ROM */
//#include "tst8080.h"
#include "4kbas32.h"

int main(void) {

    /* Init emulator */
    io_serial_init();
    panel_init();

    cpu_reset = true;
    cpu_step();
    cpu_reset = false;
    cpu_hold = true;

    sei();

    while(1) {
        /* Update emulation */
        cpu_step();

    }
}
