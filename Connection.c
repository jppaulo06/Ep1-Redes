#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>

#include "Connection.h"
#include "Frame.h"
#include "super_header.h"

// remove
#include <stdio.h>

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static Connection* new_AMQP_Connection(int socket);

static void init_connection(Connection *connection);
static void process_connection_tune_ok(Connection *connection,
                                       Connection_Tune_Ok arguments);
/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_connection(int connfd) {
  Connection* connection = new_AMQP_Connection(connfd);
  init_connection(connection);
  receive_and_process_frame(connection);
  if (connection->is_closed) {
    free(connection);
  }
}

void process_frame_connection(Connection *connection, Method_Payload payload) {
  switch ((enum IMQP_Frame_Connection)payload.method) {
  case CONNECTION_START:
    fprintf(stderr, "[WARNING] Received a not expected connection start\n");
    break;
  case CONNECTION_START_OK:
    send_connection_tune(connection);
    break;
  case CONNECTION_TUNE:
    fprintf(stderr, "[WARNING] Received a not expected connection tune ok\n");
    break;
  case CONNECTION_TUNE_OK:
    process_connection_tune_ok(connection,
                               payload.arguments.connection_tune_ok);
    break;
  case CONNECTION_OPEN:
    send_connection_open_ok(connection);
    break;
  case CONNECTION_OPEN_OK:
    fprintf(stderr, "[WARNING] Received a not expected connection open ok\n");
    break;
  case CONNECTION_CLOSE:
    send_connection_close_ok(connection);
    close_connection(connection);
    break;
  case CONNECTION_CLOSE_OK:
    fprintf(stderr, "[WARNING] Received a not expected connection close ok\n");
    break;
  default:
    fprintf(stderr, "[WARNING] Received a not expected connection method\n");
  }
}

void close_connection(Connection *connection) {
  close(connection->socket);
  connection->is_closed = 1;
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

void process_connection_tune_ok(Connection *connection,
                                Connection_Tune_Ok arguments) {
  connection->config.channel_max = arguments.channel_max;
  connection->config.frame_max = arguments.frame_max;
}

Connection* new_AMQP_Connection(int socket) {
  Connection* connection = malloc(sizeof(*connection));
  bzero(connection, sizeof(*connection));
  connection->socket = socket;
  return connection;
}

void init_connection(Connection *connection) {
  receive_protocol_header(connection);
  send_connection_start(connection);
}
