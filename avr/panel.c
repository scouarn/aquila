/* atmega2560 */

/* NOTE: Although it is designed to work with 16bit addresses,
    for now it is only used and tested with 12bit addresses */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "../emul/io.h"

#include "panel.h"

static data_t *curr_data;
static addr_t *curr_addr;

/* Global timer (mainly used for debounce) */
static uint16_t timer_ticks = 0;

/* 2ms */
#define TIMER_DELAY (0xFFFFUL - F_CPU/(64UL * 500UL))

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

/* Init external interrupts, switch inputs, timer and LED pins */
void panel_init(void) {
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

    // FIXME: "io_data_bus"
    curr_data = &cpu_A;
    curr_addr = &cpu_PC;
}

/* Read 8bit data from port A */
data_t panel_read_data(void) {
    return ~PINA;
}

/* Read address from ports C and A */
addr_t panel_read_addr(void) {
    return ~((PINC << 8) | PINA);
}

/* Read the state of the shift button */
int panel_read_shift(void) {
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

    switch (curr_nib) {
        case STAT_NIB_0: val = (cpu_hold << 3) | (cpu_wait << 2); break;
        case DATA_NIB_0: val = *curr_data >>  0; break;
        case DATA_NIB_1: val = *curr_data >>  4; break;
        case ADDR_NIB_0: val = *curr_addr >>  0; break;
        case ADDR_NIB_1: val = *curr_addr >>  4; break;
        case ADDR_NIB_2: val = *curr_addr >>  8; break;
        case ADDR_NIB_3: val = *curr_addr >> 12; break;
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
    if (panel_read_shift()) {
        (*curr_addr)++;
    }
    else {
        *curr_addr = panel_read_addr();
    }

    // FIXME: there should be a "io_data_bus"
    *curr_data = io_ram[*curr_addr];
}

/* Deposit (next) button */
ISR (INT1_vect) {
    DEBOUNCE;
    if (!cpu_hold) return;

    /* Next */
    if (panel_read_shift()) {
        (*curr_addr)++;
    }

    // FIXME: there should be a "io_data_bus"
    *curr_data = panel_read_data();
    io_ram[*curr_addr] = *curr_data;
}

/* Shift button */
// ISR (INT2_vect) {}

/* Step button */
ISR (INT3_vect) {
    DEBOUNCE;

    // FIXME: is it safe ? This is an ISR after the CPU got halted,
    // so it shouldn't be currently executing stuff
    if (cpu_hold) {
        cpu_hold = false;
        cpu_step();
        cpu_hold = true;
    }
}

/* Run/stop/reset button */
ISR (INT4_vect) {
    DEBOUNCE;

    /* Reset */
    if (panel_read_shift()) {
        // FIXME: does not work, the CPU could be executing
        // -> simply set a reset pin on the cpu and return
        cpu_reset(); 
        cpu_hold = true;
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

