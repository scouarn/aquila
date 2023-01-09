CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ggdb

SRC_DIR = source
EXE_DIR = main
BIN_DIR = bin

SRC = $(widlcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c,$(BIN_DIR)/%.o,$(SRC))

EXE_SRC = $(wildcard $(EXE_DIR)/*.c)
EXE_OBJ = $(patsubst %.c,$(BIN_DIR)/%.o,$(EXE_SRC))
EXE     = $(patsubst $(EXE_DIR)/%.c,$(BIN_DIR)/%,$(EXE_SRC))
EXE_RUN = $(patsubst $(EXE_DIR)/%.c,run_%,$(EXE_SRC))

all: $(EXE)

show:
	@echo SRC_DIR=$(SRC_DIR)
	@echo EXE_DIR=$(EXE_DIR)
	@echo BIN_DIR=$(BIN_DIR)
	@echo SRC=$(SRC)
	@echo OBJ=$(OBJ)
	@echo EXE_SRC=$(EXE_SRC)
	@echo EXE_OBJ=$(EXE_OBJ)
	@echo EXE=$(EXE)

$(EXE_RUN) : run_% : $(BIN_DIR)/%
	@$<

$(EXE) : $(BIN_DIR)/% : $(BIN_DIR)/$(EXE_DIR)/%.o $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ) $(EXE_OBJ) : $(BIN_DIR)/%.o : %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BIN_DIR)
