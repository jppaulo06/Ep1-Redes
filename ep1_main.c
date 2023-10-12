#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* modified */
#include "Connection.h"
#include <assert.h>

#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096
#define AMQP_PORT 5672

#include "Connection.h"

const SOAG son_of_a_greve = {
    .config =
        {
            .channel_max = 1,
            .frame_max = MAX_FRAME_SIZE,
            .heartbeat = 0,
        },
    .meta =
        {
            .major_version = 0,
            .minor_version = 9,
            .cluster_name = "Is it AMQP or IMQP??",
            .license = "Do What the Fuck You Want To Public License",
            .product = "Son of a Greve IMQP",
            .mechanisms = "PLAIN AMQP LAIN",
            .locales = "pt_BR en_US",
        },
};

int main(int argc, char **argv) {
  int listenfd, connfd;
  struct sockaddr_in servaddr = {0};

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
    if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1) {
      perror("accept :(\n");
      exit(5);
    }

    printf("[Uma conexão aberta] %d\n", connfd);

    process_connection(connfd);
  }
  exit(0);
}
