#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "cpu.h"

/* Defining the machine */
static cpu_t  cpu;
static data_t ram[RAM_SIZE];
static addr_t addr_bus, addr_reg;
static data_t data_bus, data_reg;

#define TTY_PORT 0
//sock_fd tty_sock;

static data_t load(addr_t addr) {
    addr_bus = addr;
    data_bus = ram[addr];
    return data_bus;
}

static void store(addr_t addr, data_t data) {
    addr_bus  = addr;
    data_bus  = data;
    ram[addr] = data;
}

static data_t input(port_t port) {
    addr_bus = port;

    switch(port) {
        case TTY_PORT:
            printf("Input: ");
            fflush(stdout);
            scanf("%c", &data_bus);
        break;

        default: 
            data_bus = 0x00;
        break;
    }

    return data_bus;
}

static void output(port_t port, data_t data) {
    addr_bus = port;
    data_bus = data;

    switch(port) {
        case TTY_PORT:
            printf("Output : %c\n", data);
        break;

        default: break;
    }
}

static void print(void) {
    printf("Data: $%02hx\n", data_bus);
    printf("Addr: $%04hx\n", addr_bus);
}

static void prompt(void) {
    printf("aquila> ");
    fflush(stdout);
}

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
    "    reg            : display registers\n"
;

/* Catch SIGINT when running */
static volatile bool running = false;
static void run_interrupt(int sig) {
    (void)sig;
    if (running) {
        running = false;
    }
    else {
        printf("\nInterrupt\n");
        prompt();
    }
}


/* Simulator shell */
int main(void) {
    char buf[1024];
    char *tok;
    const char sep[] = " \n";

    /* Init machine */
    cpu.load   = load;
    cpu.store  = store;
    cpu.input  = input;
    cpu.output = output;
    memset(ram, 0, RAM_SIZE);
    signal(SIGINT, run_interrupt);

    while (1) {

        /* Get line and quit on EOF */
        prompt();
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
            print();
        }

        /* Examine */
        else if (strcmp("ex", tok) == 0) {

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hx", &addr_reg) != 1) {
                printf("Expected ADDR\n");
                continue;
            }

            addr_bus = addr_reg;
            data_bus = ram[addr_bus];
            print();
        }

        /* Examine next */
        else if (strcmp("exn",    tok) == 0
              || strcmp("exnext", tok) == 0) {
            addr_bus = ++addr_reg;
            data_bus = ram[addr_bus];
            print();
        }

        /* Deposit */
        else if (strcmp("dep", tok) == 0) {

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hhx", &data_reg) != 1) {
                printf("Expected DATA\n");
                continue;
            }

            addr_bus = addr_reg;
            data_bus = data_reg;
            ram[addr_bus] = data_bus;
            print();
        }

        /* Deposit next */
        else if (strcmp("depn",    tok) == 0
              || strcmp("depnext", tok) == 0) {

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hhx", &data_reg) != 1) {
                printf("Expected DATA\n");
                continue;
            }

            addr_bus = ++addr_reg;
            data_bus = data_reg;
            ram[addr_bus] = data_bus;
            print();
        }

        /* Step */
        else if (strcmp("s",    tok) == 0
              || strcmp("step", tok) == 0) {
            cpu_step(&cpu);
            print();
        }

        /* Run */
        else if (strcmp("run", tok) == 0) {
            running = true;
            while (running) {
                cpu_step(&cpu);
            }
            print();
        }

        /* Reset */
        else if (strcmp("reset", tok) == 0) {
            cpu_reset(&cpu);
        }

        /* Hard reset */
        else if (strcmp("reboot", tok) == 0) {
            cpu_reset(&cpu);
            memset(ram, 0, sizeof(ram));
        }

        /* Display registers */
        else if (strcmp("reg", tok) == 0) {
            cpu_dump(&cpu, stdout);
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
