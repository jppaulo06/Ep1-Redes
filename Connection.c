#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Connection.h"
#include "Frame.h"
#include "super_header.h"

// remove
#include <stdio.h>

/*====================================*/
/* EXTERN VARIABLES */
/*====================================*/

extern SOAG son_of_a_greve;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static Connection *new_AMQP_Connection(int socket);

static uint64_t init_connection(Connection *connection);
static uint64_t process_connection_tune_ok(Connection *connection,
                                           Connection_Tune_Ok arguments);
/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void *process_connection(void *fd) {
  uint64_t connfd = (uint64_t)fd;
  uint64_t flags = 0;
  Connection *connection = new_AMQP_Connection(connfd);

  flags |= init_connection(connection);

  if (!(flags & (ERRORS | CONNECTION_ENDED)))
    flags |= receive_and_process_frame(connection);

  close_connection(connection);
  free(connection);

  print_errors_or_warnings(flags);

  return NULL;
}

uint64_t process_frame_connection(Connection *connection,
                                  Method_Payload payload) {
  uint64_t flags = 0;
  switch ((enum IMQP_Frame_Connection)payload.method) {
  case CONNECTION_START_OK:
    send_connection_tune(connection);
    break;
  case CONNECTION_TUNE_OK:
    flags |= process_connection_tune_ok(connection,
                                        payload.arguments.connection_tune_ok);
    break;
  case CONNECTION_OPEN:
    send_connection_open_ok(connection);
    break;
  case CONNECTION_CLOSE:
    send_connection_close_ok(connection);
    close_connection(connection);
    flags |= CONNECTION_ENDED;
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
  }
  return flags;
}

void close_connection(Connection *connection) {
  close(connection->socket);
  connection->closed = true;
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_connection_tune_ok(Connection *connection,
                                    Connection_Tune_Ok arguments) {
  connection->config.channel_max = arguments.channel_max;
  connection->config.frame_max = arguments.frame_max;
  connection->config.heartbeat = arguments.heartbeat;

  const Config *config = &(son_of_a_greve.config);

  if (connection->config.channel_max > config->channel_max ||
      connection->config.frame_max > config->frame_max ||
      connection->config.heartbeat > config->heartbeat) {
    return INVALID_TUNE;
  }

  return NO_ERROR;
}

Connection *new_AMQP_Connection(int socket) {
  Connection *connection = malloc(sizeof(*connection));
  bzero(connection, sizeof(*connection));
  connection->socket = socket;

  connection->meta.major_version = MAJOR_VERSION;
  connection->meta.minor_version = MINOR_VERSION;
  strcpy(connection->meta.cluster_name, son_of_a_greve.meta.cluster_name);
  strcpy(connection->meta.license, son_of_a_greve.meta.license);
  strcpy(connection->meta.product, son_of_a_greve.meta.product);
  strcpy(connection->meta.mechanisms, son_of_a_greve.meta.mechanisms);
  strcpy(connection->meta.locales, son_of_a_greve.meta.locales);

  connection->config.channel_max = son_of_a_greve.config.channel_max;
  connection->config.frame_max = son_of_a_greve.config.frame_max;
  connection->config.heartbeat = son_of_a_greve.config.heartbeat;

  return connection;
}

uint64_t init_connection(Connection *connection) {
  uint64_t flags = NO_ERROR;

  flags |= receive_protocol_header(connection);

  if (flags & CONNECTION_ENDED)
    return flags;

  if (flags & INVALID_PROTOCOL_HEADER) {
    send_protocol_header(connection);
    flags = receive_protocol_header(connection);
    if (flags)
      return flags;
  }

  send_connection_start(connection);
  return flags;
}
