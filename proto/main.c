#include <string.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "emul/cpu.h"
#include "device.h"


int main(void) {

    /* Init microcontroller */
    device_init();

    /* Init emulator */
    cpu_reset = true;
    cpu_step();
    cpu_hold = true;

    sei();

    while(1) {
        /* Update emulation */
        cpu_step();

    }
}
