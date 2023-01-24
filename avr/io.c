#include <avr/io.h>

#include "../emul/io.h"

data_t io_load(addr_t addr) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return 0x00;
#endif

    return io_ram[addr];
}

void io_store(addr_t addr, data_t data) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return;
#endif
    io_ram[addr] = data;
}

data_t io_input(port_t port) {
    PORTB |= _BV(PORTB7);
    data_t data = 0x00;

    switch (port) {

        /* Translate the USART status byte into a 88-2 SIO status byte */
        case SIO_STATUS:

            /* Rx busy */
            if ((UCSR0A & _BV(RXC0)) == 0) {
                data |= 0x03;
            }

            /* Tx busy */
            if ((UCSR0A & _BV(UDRE0)) == 0) {
                data |= 0x80;
            }

            /* Error bits can also be hooked up */
        break;

        case SIO_DATA:
            /* Wait until there is data to read */
            while ((UCSR0A & _BV(RXC0)) == 0) {}

            /* Receive */
            data = UDR0;
        break;

        default: break;
    }

    PORTB &= ~_BV(PORTB7);
    return data;
}

void io_output(port_t port, data_t data) {
    PORTB |= _BV(PORTB7);

    switch (port) {

        /* Control register of the SIO card */
        case SIO_STATUS:
            /* */
        break;

        case SIO_DATA:
            /* Wait until there is space to write */
            while ((UCSR0A & _BV(UDRE0)) == 0) {}

            /* Send: chop the msbit because 4k basic does things to them */
            UDR0 = data & 0x7f;
        break;

        default: break;
    }

    PORTB &= ~_BV(PORTB7);
}


void io_serial_init(void) {

    /* Enable Rx/Tx */
    UCSR0B |= _BV(RXEN0)  | _BV(TXEN0);

    /* 8bit character size */
    UCSR0C |= _BV(UCSZ10) | _BV(UCSZ00);

    /* Rate */
    UBRR0L = BAUD_PRESCALLER & 0xff;
    UBRR0H = BAUD_PRESCALLER >> 8;

}