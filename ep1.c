#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

/* modified */
#include "ampq.h"
#include <assert.h>

#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096

int main (int argc, char **argv) {
    int listenfd, connfd;
    struct sockaddr_in servaddr = {0};
    pid_t childpid;
    char recvbuffer[MAXLINE + 1];
    ssize_t n;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket :(\n");
        exit(2);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(AMQP_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind :(\n");
        exit(3);
    }

    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen :(\n");
        exit(4);
    }

    printf("[Servidor no ar. Aguardando conexões na porta %d]\n", AMQP_PORT);
    printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");

	for (;;) {
        if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
            perror("accept :(\n");
            exit(5);
        }
        
        printf("[Uma conexão aberta]\n");

        /* Initialize connection */

        Connection *connection = new_AMQP_Connection(connfd);

        int method = connection_get_method(connection);

        switch(method){
          case 
        }


        if((n = read(connfd, recvbuffer, MAXLINE)) > 0) {
          printf("%s", recvbuffer);
        }
        else {
          assert(0 && "Read error not implemented yet");
        }

        close(connfd);
    }
    exit(0);
}
