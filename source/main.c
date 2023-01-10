#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "cpu.h"

#define RAM_SIZE 0x10000

/* Help message */
static const char HELP[] =
    "    q, quit        : quit\n"
    "    h, help        : print this message\n"
    "    p, print       : print the current status\n"
    "    ex ADDR        : examine at address\n"
    "    exn, exnext    : examine next\n"
    "    dep DATA       : deposit data\n"
    "    depn DATA      : deposit next\n"
    "    step           : step single instruction\n"
    "    run            : run until sigint (Ctrl+C) is received\n"
    "    reset          : reset CPU\n"
    "    reboot         : reset CPU and RAM (power off then on)\n"
;

/* Memory and IO */
static uint8_t  ram[RAM_SIZE] = {0};
static uint16_t curr_addr = 0;

static void on_write(cpu_t *cpu, addr_t addr, data_t data) {
    (void)cpu;
    curr_addr = addr;
    ram[curr_addr] = data;
}

static data_t on_read(cpu_t *cpu, addr_t addr) {
    (void)cpu;
    curr_addr = addr;
    return ram[curr_addr];
}

/*
static void on_output(cpu_t *cpu, port_t port, data_t data) {
    (void)cpu;
    curr_addr = addr;
    ram[curr_addr] = data;
}

static data_t on_input(cpu_t *cpu, port_t port) {
    (void)cpu;
    curr_addr = addr;
    return ram[curr_addr];
}
*/

/* Simulator shell */
int main(void) {
    char buf[1024];
    char *tok;
    const char sep[] = " \n";

    /* Init CPU */
    cpu_t cpu;
    cpu_reset(&cpu);
    cpu.on_write = on_write;
    cpu.on_read  = on_read;

    while (1) {

        /* Prompt */
        printf("aquila> ");
        fflush(stdout);

        /* Get line and quit on EOF */
        if (NULL == fgets(buf, sizeof(buf), stdin)) {
            printf("\n");
            goto end_while;
        }

        /* Split by spaces */
        tok = strtok(buf, sep);
        if (tok == NULL) {
            continue;
        }

        /* Quit */
        else if (strcmp("q",    tok) == 0
              || strcmp("quit", tok) == 0) {
            goto end_while;
        }

        /* Help */
        else if (strcmp("h",    tok) == 0
                || strcmp("help", tok) == 0) {
            printf("%s", HELP);
        }

        /* Print */
        else if (strcmp("p",     tok) == 0
              || strcmp("print", tok) == 0) {
print:
            printf("Data: $%02hx\n", ram[curr_addr]);
            printf("Addr: $%04hx\n", curr_addr);
        }

        /* Examine */
        else if (strcmp("ex", tok) == 0) {
            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hx", &curr_addr) != 1) {
                printf("Expected ADDR\n");
                continue;
            }
            goto print;
        }

        /* Examine next */
        else if (strcmp("exn",    tok) == 0
              || strcmp("exnext", tok) == 0) {
            curr_addr++;
            goto print;
        }

        /* Deposit */
        else if (strcmp("dep", tok) == 0) {
deposit:
            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hhx", &ram[curr_addr]) != 1) {
                printf("Expected DATA\n");
                continue;
            }
            goto print;
        }

        /* Deposit next */
        else if (strcmp("depn",    tok) == 0
              || strcmp("depnext", tok) == 0) {
            curr_addr++;
            goto deposit;
        }

        /* Step */
        else if (strcmp("s",    tok) == 0
              || strcmp("step", tok) == 0) {
            cpu_step(&cpu);
            goto print;
        }

        /* Run */
        else if (strcmp("run", tok) == 0) {
            printf("Run unimplemented\n"); // TODO
        }

        /* Reset */
        else if (strcmp("reset", tok) == 0) {
reset:
            curr_addr = 0;
            cpu_reset(&cpu);
        }

        /* Hard reset */
        else if (strcmp("reboot", tok) == 0) {
            memset(ram, 0, RAM_SIZE);
            goto reset;
        }

        /* Error */
        else {
            printf("Unknown command '%s', type 'h' or 'help' for help\n", tok);
        }

    }

end_while:
    printf("Bye!\n");
    return 0;
}
