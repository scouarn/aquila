#ifndef DEVICE_H
#define DEVICE_H

#include "emul/cpu.h"

#define BAUD_USB 9600U
#define BAUD_MINITEL 4800U

#define SIO_STATUS_PORT 0
#define SIO_DATA_PORT   1
#define SENSE_PORT    255

/* Setup interrupts, timers, serial and GPIO */
void device_init(void);


#endif
