#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SA struct sockaddr
#define DEFAULT_PORT 8080

int ser_sock = -1;
int cli_sock = -1;

void connect_cli(void) {
    printf("Waiting for the client to connect...\n");
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);

    /* Waiting for a client */
    if ((cli_sock = accept(ser_sock, (SA*)&addr, &len)) < 0) {
        perror("accept");
        close(ser_sock);
        exit(1);
    }

    /* Writing the client's IP */
    char buff[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, buff, sizeof(buff)) == NULL) {
        perror("inet_ntop");
        close(ser_sock);
        close(cli_sock);
        exit(1);
    }

    printf("Connected to %s\n", buff);
}

char input(void) {
try_input:
    if (cli_sock == -1) {
        connect_cli();
    }

    /* Reading */
    char data;
    ssize_t received = read(cli_sock, &data, 1);
    if (received <= 0) {
        printf("Connection closed while reading\n");
        cli_sock = -1;
        goto try_input;
    }

    return data;
}

void output(char data) {
try_output:
    if (cli_sock == -1) {
        connect_cli();
    }

    /* Writing */
    ssize_t sent = send(cli_sock, &data, 1, MSG_NOSIGNAL);
    if (sent <= 0) {
        printf("Connection closed while writing\n");
        cli_sock = -1;
        goto try_output;
    }
}


int main(int argc, char **argv) {

    /* Parsing args */
    in_port_t port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        if (!strcmp("-p", argv[i])) {
            if (++i >= argc) goto usage;
            port = atoi(argv[i]);
        }
        else if (!strcmp("-h", argv[i])) {
            goto usage;
        }
        else {
            goto usage;
        }
    }


    /* */
    struct sockaddr_in ser_addr;
    ser_addr.sin_family      = AF_INET;
    ser_addr.sin_port        = htons(port);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* */
    if ((ser_sock = socket(ser_addr.sin_family, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* */
    if (bind(ser_sock, (SA*)&ser_addr, sizeof(ser_addr)) != 0) {
        perror("bind");
        close(ser_sock);
        exit(1);
    }

    /* */
    if (listen(ser_sock, 1) != 0) {
        perror("listen");
        close(ser_sock);
        exit(1);
    }

    for (int it = 0; 1; it++) {
        printf("it=%d\n", it);

        if (it % 3 == 0) {
            output('3');
            printf("sent: '%c'\n", '3');
        }
        if (it % 5 == 0) {
            printf("received ");
            fflush(stdout);
            printf("'%c'\n", input());
        }

        sleep(1);
    }

    close(ser_sock);
    close(cli_sock);
    return EXIT_SUCCESS;

usage:
    fprintf(stderr,
        "Usage: %s [-p <port>]\n"
        "    -p    Set the TCP port for the TTY output (default 8080)\n",
        argv[0]
    );
    return EXIT_FAILURE;
}
