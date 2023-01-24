#ifndef PANEL_H
#define PANEL_H

#include "../emul/cpu.h"

/* Setup interrupts, timers and GPIO for the front panel */
void panel_init(void);

data_t panel_read_data  (void);
addr_t panel_read_addr  (void);
int    panel_read_shift (void);

#endif
