#ifndef DEVICE_H
#define DEVICE_H

#include "../emul/cpu.h"

#define BAUD_USB 9600U
#define BAUD_MINITEL 4800U

/* Setup interrupts, timers, serial and GPIO */
void device_init(void);

/*
data_t device_read_sense (void);
data_t device_read_data  (void);
addr_t device_read_addr  (void);
int    device_read_shift (void);
*/

#endif
