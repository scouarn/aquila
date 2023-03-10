/* atmega2560 */

/* NOTE: Although it is designed to work with 16bit addresses,
    for now it is only used and tested with 12bit addresses */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "device.h"
#include "emul/cpu.h"

#define BAUD_PRESCALE(BAUD_RATE) ((F_CPU / (16UL*BAUD_RATE))-1)


data_t io_data_bus = 0x00;
addr_t io_addr_bus = 0x0000;


#define RAM_SIZE 4096
data_t ram[RAM_SIZE] = {

/* Boot ROM */
#include "rom_4kbas32.h"

};


/* 2ms */
#define TIMER_DELAY (0xFFFFUL - F_CPU/(64UL * 500UL))

/* Global timer (mainly used for debounce) */
static uint16_t timer_ticks = 0;

/* Multiplexing: Bits of port L used to address groups of 4 LEDs */
enum led_nibbles {
    DATA_NIB_1,
    DATA_NIB_0,
    ADDR_NIB_0,
    STAT_NIB_0,
    ADDR_NIB_1,
    ADDR_NIB_2,
    ADDR_NIB_3,
    NIB_COUNT
};


/*
    Device IO functions
*/


/* Init external interrupts, switch inputs, timer and LED pins */
void device_init(void) {
    /* Enable input with pullup on pins
        INT0 INT1 [INT2] INT3 INT4 [INT5]
        21   20    19    18   2    [3   ]
        D0   D1    D2    D3   E4   [E5  ]

        Note: D2 is the shift button which is read directly (no interrupt).
        Note: E5/INT5 is not used at all.
    */
    DDRD  &= ~(_BV(PORTD0) | _BV(PORTD1) | _BV(PORTD2) | _BV(PORTD3));
    DDRE  &= ~(_BV(PORTE4) | _BV(PORTE5));
    PORTD |=   _BV(PORTD0) | _BV(PORTD1) | _BV(PORTD2) | _BV(PORTD3);
    PORTE |=   _BV(PORTE4) /* | _BV(PORTE5) */;

    /* Trigger on rising edge (button release) */
    EICRA |= _BV(ISC01) | _BV(ISC01);
    EICRA |= _BV(ISC11) | _BV(ISC11);
    /* EICRA |= _BV(ISC21) | _BV(ISC21); */
    EICRA |= _BV(ISC31) | _BV(ISC31);
    EICRB |= _BV(ISC41) | _BV(ISC41);
    /* EICRB |= _BV(ISC51) | _BV(ISC51); */

    /* Enable INT0-5 */
    EIMSK |= _BV(INT0) | _BV(INT1) /* | _BV(INT2) */
          |  _BV(INT3) | _BV(INT4) /* | _BV(INT5) */;


    /* 16bit timer, divide by 64 */
    TCCR1A = 0x00;
    TCNT1  = 0x00;
    TCCR1B |= _BV(CS11) | _BV(CS10);
    TIMSK1 |= _BV(TOIE1);


    /* Address/data switches:
        Enable input with pullup on port A (pins 22:29)
        and C (pins 37:30) */
    DDRA = 0x00; PORTA = 0xff;
    DDRC = 0x00; PORTC = 0xff;


    /* LED multiplexer 4 "value" pins
        and 6 "select" pins as output */
    DDRB = 0xff; PORTB = 0x00;
    DDRL = 0xff; PORTL = 0xff;


    /*
        Init serial for USB (port 0)
    */

    /* Enable Rx/Tx */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);

    /* 8bit char, 1 stop bit, no parity */
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

    /* Baud rate */
    UBRR0L = BAUD_PRESCALE(BAUD_USB) & 0xff;
    UBRR0H = BAUD_PRESCALE(BAUD_USB) >> 8;


    /*
        Init serial for Minitel
        TODO: send init codes to the minitel at 1200 baud then switch to 4800
    */

    /* Enable Rx/Tx */
    UCSR2B = _BV(RXEN2) | _BV(TXEN2);

    /* 7bit char, 1 stop bit, even parity */
    UCSR2C = _BV(UCSZ21) | _BV(UPM21);

    /* Baud rate */
    UBRR2L = BAUD_PRESCALE(BAUD_MINITEL) & 0xff;
    UBRR2H = BAUD_PRESCALE(BAUD_MINITEL) >> 8;


}

/* Read 8bit data from port A */
data_t device_read_data(void) {
    return ~PINA;
}

/* On the real Altair the sense switches are the upper 8bits */
data_t device_read_sense(void) {
    return ~PINA;
}

/* Read address from ports C and A */
addr_t device_read_addr(void) {
    return ~((PINC << 8) | PINA);
}

/* Read the state of the shift button */
int device_read_shift(void) {
    return !(PIND & _BV(PIND2));
}

/* Timer interrupt: update ticks counter and LED multiplexer */
ISR(TIMER1_OVF_vect) {
    TCNT1 = TIMER_DELAY;
    timer_ticks++;

    static int curr_nib = 0;
    static uint8_t count = 0;
    if (timer_ticks % 100 == 0) count ++;

    uint8_t val;

    /* Read the state of the CPU and buses */
    switch (curr_nib) {
        case STAT_NIB_0: val = (cpu_hold << 3) | (!cpu_ready << 2); break;
        case DATA_NIB_0: val = io_data_bus >>  0; break;
        case DATA_NIB_1: val = io_data_bus >>  4; break;
        case ADDR_NIB_0: val = io_addr_bus >>  0; break;
        case ADDR_NIB_1: val = io_addr_bus >>  4; break;
        case ADDR_NIB_2: val = io_addr_bus >>  8; break;
        case ADDR_NIB_3: val = io_addr_bus >> 12; break;
        default: val = 0x00; break;
    }

    PORTB = (~val) & 0x0f;
    PORTL = 1U << curr_nib;

    if (++curr_nib > NIB_COUNT) curr_nib = 0;
}


/* Abort ISR if a tick hasn't passed since last press */
#define DEBOUNCE \
    static uint16_t debounce = 0; \
    if (debounce == timer_ticks) return;

/* Examine (next) button */
ISR (INT0_vect) {
    DEBOUNCE;
    if (!cpu_hold) return;

    /* Next */
    if (device_read_shift()) {
        io_addr_bus++;
    }
    else {
        io_addr_bus = device_read_addr();
    }

    /* Update state of the buses */
    cpu_PC = io_addr_bus;
    io_data_bus = ram[io_addr_bus];
}

/* Deposit (next) button */
ISR (INT1_vect) {
    DEBOUNCE;
    if (!cpu_hold) return;

    /* Next */
    if (device_read_shift()) {
        io_addr_bus++;
    }

    /* Update state of the buses */
    io_data_bus = device_read_data();

    /* Update RAM content */
    ram[io_addr_bus] = io_data_bus;
}

/* Shift button */
// ISR (INT2_vect) {}

/* Step button */
ISR (INT3_vect) {
    DEBOUNCE;

    // FIXME: is it safe ? This is an ISR after the CPU got halted,
    // so it shouldn't be currently executing stuff
    // FIXME: When executing IO instruction and waits the LEDs aren't updated
    // since interrupts in the host are disabled
    if (cpu_hold) {
        cpu_hold = false;
        cpu_step();
        cpu_hold = true;
        io_addr_bus = cpu_PC;
    }
}

/* Run/stop/reset button */
ISR (INT4_vect) {
    DEBOUNCE;

    /* Reset */
    if (device_read_shift()) {
        // FIXME: Not sure if it really works
        cpu_reset = true; // Reset occurs on next step
        cpu_hold = true; // FIXME: does reset on the real 8080 bypasses hold ?
        // On real Altair reset does not set hold but wait...

        // FIXME: this does not work because the instruction beeing
        // executed has to end and can still put things on the bus
        io_data_bus = 0x00;
        io_addr_bus = 0x0000;
    }

    /* Run */
    else if (cpu_hold) {
        cpu_hold = false;
    }

    /* Stop */
    else {
        cpu_hold = true;
    }
}

/* Unused */
// ISR (INT5_vect) {}


/*
    CPU IO functions
*/

data_t cpu_load(addr_t addr) {
    if (addr >= RAM_SIZE) return 0x00;

    io_addr_bus = addr;
    io_data_bus = ram[addr];
    return io_data_bus;
}

void cpu_store(addr_t addr, data_t data) {
    if (addr >= RAM_SIZE) return;

    io_addr_bus  = addr;
    io_data_bus  = data;
    ram[addr] = data;
}

/* NOTE:
    We receive from ONE of the two serial ports (USB/Minitel),
    But we send to BOTH serial ports.
*/
data_t cpu_input(port_t port) {
    io_addr_bus = (port << 8) | port; // According to datasheet

    switch (port) {

        /* Translate the USART status byte into a 88-2 SIO status byte */
        case SIO_STATUS_PORT:


            /* Rx busy */
            /* If both of them are busy then we can't receive */
            if ((UCSR0A & _BV(RXC0)) == 0
             && (UCSR2A & _BV(RXC2)) == 0) {
                io_data_bus |= 0x03;
            }

            /* Tx busy */
            /* If one of them is busy then we can't send */
            if ((UCSR0A & _BV(UDRE0)) == 0
             || (UCSR2A & _BV(UDRE2)) == 0) {
                io_data_bus |= 0x80;
            }

            /* Error bits can also be hooked up */
        break;

        case SIO_DATA_PORT:

            /* Wait until there is data to read in one of the two ports */
            while (1) {

                /* Receive from USB */
                if (UCSR0A & _BV(RXC0)) {
                    io_data_bus = UDR0;
                    break;
                }

                /* Receive from Minitel */
                else if (UCSR2A & _BV(RXC2)) {
                    io_data_bus = UDR2;
                    break;
                }

                else {
                    cpu_ready = false;
                }
            }

            cpu_ready = true;
        break;

        case SENSE_PORT:
            io_data_bus = device_read_sense();
        break;

        default: break;
    }

    return io_data_bus;
}

void cpu_output(port_t port, data_t data) {
    io_addr_bus = (port << 8) | port; // According to datasheet
    io_data_bus = data;

    switch (port) {

        /* Control register of the SIO card */
        case SIO_STATUS_PORT:
            /* */
        break;

        case SIO_DATA_PORT:
            /* Wait until there is space to write to both ports */
            while ((UCSR0A & _BV(UDRE0)) == 0
               &&  (UCSR2A & _BV(UDRE2)) == 0)
            {
                cpu_ready = false;
            }
            cpu_ready = true;

            UDR0 = data;
            UDR2 = data;
        break;

        case SENSE_PORT:
            /* Nothing to do */
        break;

        default: break;
    }

}
