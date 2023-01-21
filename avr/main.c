#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#include "../emul/cpu.h"
#include "../emul/io.h"
//#include "tst8080.h"
#include "4kbas32.h"

int main(void) {

    /* Init emulator */
    io_serial_init();
    cpu_reset();

    /* Enable output pin 13 */
    DDRB |= _BV(PORTB7);
    PORTB = 0x00;

    while(1) {
        /* Update emulation */
        cpu_step();

        /* Set the hold LED */
        if (cpu_hold) {
            //PORTB |= _BV(PORTB7);
        }
        else {
            //PORTB = 0x00;
        }

        //_delay_us(1000);
    }
}
