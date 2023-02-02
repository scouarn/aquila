SRC = $(wildcard *.c) emul/cpu.c
OBJ = $(patsubst %.c,$(OUT)/%.o,$(SRC))

ELF = $(OUT)/main.elf
HEX = $(OUT)/main.hex

DEVICE = atmega2560
FREQ = 16000000L
PORT = /dev/ttyACM0
RATE = 115200
PROGRAMMER = wiring

CC = avr-gcc

CFLAGS = -Os -Wall -Wextra -pedantic -std=gnu99 -I..\
		-mmcu=$(DEVICE) -DF_CPU=$(FREQ) \
		-ffunction-sections -fdata-sections \
		--param=min-pagesize=0 # PORTB &= ~X : bugged warning in GCC > 12.x

LDFLAGS = -Wl,--gc-sections

all: $(HEX)

$(ELF): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/emul/%.o: $(ROOT)/emul/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

%.hex: %.elf
	@mkdir -p $(@D)
	avr-objcopy -O ihex -R .eeprom $< $@

upload: $(HEX)
	avrdude -D -V \
		-P $(PORT) -c $(PROGRAMMER) -p $(DEVICE) \
		-U flash:w:$(HEX):i

