BIN = $(PWD)/bin
OUT = $(BIN)/$(CURMODULE)

# Options
CC = gcc
CFLAGS  += -Werror -Wall -Wextra -pedantic -I$(PWD)
LDFLAGS +=
LDLIBS  +=

# Additional options for debug/release
#CFLAGS += -O3
CFLAGS += -ggdb

# Build directories
MKDIR := mkdir -p

# Linking
define link
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
endef

$(OUT)/%.o : %.c
	$(call link)

# Compilation
define compile
	@$(MKDIR) $(@D)
	$(CC) -c $(CFLAGS) -o $@ $<
endef

$(OUT)/%.o : %.c
	$(call compile)
