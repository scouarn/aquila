#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "machine.h"

static mac_t mac;

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

static void print(void) {
    printf("Data: $%02hx\n", mac.data_bus);
    printf("Addr: $%04hx\n", mac.addr_bus);
}

static void prompt(void) {
    printf("aquila> ");
    fflush(stdout);
}


/* Status */
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
    mac_init(&mac);
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
            addr_t addr;

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hx", &addr) != 1) {
                printf("Expected ADDR\n");
                continue;
            }

            mac_ex(&mac, addr);
            print();
        }

        /* Examine next */
        else if (strcmp("exn",    tok) == 0
              || strcmp("exnext", tok) == 0) {
            mac_ex_next(&mac);
            print();
        }

        /* Deposit */
        else if (strcmp("dep", tok) == 0) {
            data_t data;

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hhx", &data) != 1) {
                printf("Expected DATA\n");
                continue;
            }

            mac_dep(&mac, data);
            print();
        }

        /* Deposit next */
        else if (strcmp("depn",    tok) == 0
              || strcmp("depnext", tok) == 0) {
            data_t data;

            tok = strtok(NULL, sep);
            if (tok == NULL || sscanf(tok, "%hhx", &data) != 1) {
                printf("Expected DATA\n");
                continue;
            }

            mac_dep_next(&mac, data);
            print();
        }

        /* Step */
        else if (strcmp("s",    tok) == 0
              || strcmp("step", tok) == 0) {
            mac_step(&mac);
            print();
        }

        /* Run */
        else if (strcmp("run", tok) == 0) {
            running = true;
            while (running) {
                mac_step(&mac);
            }
            print();
        }

        /* Reset */
        else if (strcmp("reset", tok) == 0) {
            mac_reset(&mac);
        }

        /* Hard reset */
        else if (strcmp("reboot", tok) == 0) {
            mac_init(&mac);
        }

        /* Display registers */
        else if (strcmp("reg", tok) == 0) {
            cpu_dump(&mac.cpu, stdout);
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
