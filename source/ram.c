
#include "ram.h"

size_t ram_load(data_t *ram, addr_t offset, FILE *fp) {
    fread(ram + LOAD_ADDR, 1, sizeof(ram) - LOAD_ADDR, fin);
}
    printf("==> Loaded %zu bytes from %s\n", loaded, argv[1]);
