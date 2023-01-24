#include <avr/io.h>

#include "../emul/io.h"

#include "panel.h"


data_t io_data_bus = 0x00;
addr_t io_addr_bus = 0x0000;


data_t io_load(addr_t addr) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return 0x00;
#endif
    io_addr_bus = addr;
    io_data_bus = io_ram[addr];
    return io_data_bus;
}

void io_store(addr_t addr, data_t data) {
#if RAM_SIZE < 0xffff
    if (addr >= RAM_SIZE) return;
#endif
    io_addr_bus  = addr;
    io_data_bus  = data;
    io_ram[addr] = data;
}

data_t io_input(port_t port) {
    io_addr_bus = (port << 8) | port; // According to datasheet

    switch (port) {

        /* Translate the USART status byte into a 88-2 SIO status byte */
        case SIO_STATUS_PORT:

            /* Rx busy */
            if ((UCSR0A & _BV(RXC0)) == 0) {
                io_data_bus |= 0x03;
            }

            /* Tx busy */
            if ((UCSR0A & _BV(UDRE0)) == 0) {
                io_data_bus |= 0x80;
            }

            /* Error bits can also be hooked up */
        break;

        case SIO_DATA_PORT:
            /* Wait until there is data to read */
            while ((UCSR0A & _BV(RXC0)) == 0) {
                cpu_ready = false;
            }
            cpu_ready = true;

            /* Receive */
            io_data_bus = UDR0;
        break;

        case SENSE_PORT:
            io_data_bus = panel_read_sense();
        break;

        default: break;
    }

    return io_data_bus;
}

void io_output(port_t port, data_t data) {
    io_addr_bus = (port << 8) | port; // According to datasheet
    io_data_bus = data;

    switch (port) {

        /* Control register of the SIO card */
        case SIO_STATUS_PORT:
            /* */
        break;

        case SIO_DATA_PORT:
            /* Wait until there is space to write */
            while ((UCSR0A & _BV(UDRE0)) == 0) {
                cpu_ready = false;
            }
            cpu_ready = true;

            /* Send: chop the msbit because 4k basic does things to them */
            UDR0 = data & 0x7f;
        break;

        case SENSE_PORT:
            /* Nothing to do */
        break;

        default: break;
    }

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
