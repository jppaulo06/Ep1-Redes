#include <sys/socket.h>
#include <unistd.h>

#include "Connection.h"
#include "Frame.h"
#include "super_header.h"

// remove
#include <stdio.h>

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Connection {
  CONNECTION_START = 10,
  CONNECTION_START_OK,
  CONNECTION_TUNE = 30,
  CONNECTION_TUNE_OK,
  CONNECTION_OPEN = 40,
  CONNECTION_OPEN_OK,
  CONNECTION_CLOSE = 50,
  CONNECTION_CLOSE_OK,
};

/*====================================*/
/* EXTERNAL VARIABLES */
/*====================================*/

extern const char *FAKE_CONNECTION_START;
extern const int FAKE_CONNECTION_START_SIZE;

extern const char *FAKE_CONNECTION_TUNE;
extern const int FAKE_CONNECTION_TUNE_SIZE;

extern const char *FAKE_CONNECTION_OPEN_OK;
extern const int FAKE_CONNECTION_OPEN_OK_SIZE;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

Connection *new_AMQP_Connection(int socket);

static void init_connection(Connection *connection);
static void make_connection_config(Connection *connection,
                                   IMQP_Byte *arguments);
static void receive_protocol_header(Connection *connection);

static void send_connection_start(Connection *connection);
static void send_connection_tune(Connection *connection);
static void send_connection_open_ok(Connection *connection);
static void send_connection_start(Connection *connection);
static void send_connection_close_ok(Connection *connection);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_connection(int connfd) {
  Connection *connection = new_AMQP_Connection(connfd);
  process_frame(connection);
  if(connection->is_closed){
    free_connection(connection);
  }
}

void process_frame_connection(Connection *connection, IMQP_Frame *frame) {
  IMQP_Byte *arguments = frame->payload.method_pl.arguments;
  switch ((enum IMQP_Frame_Connection)frame->payload.method_pl.method) {
  case CONNECTION_START:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case CONNECTION_START_OK:
    send_connection_tune(connection);
    break;
  case CONNECTION_TUNE:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case CONNECTION_TUNE_OK:
    make_connection_config(connection, arguments);
    break;
  case CONNECTION_OPEN:
    send_connection_open_ok(connection);
    break;
  case CONNECTION_OPEN_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case CONNECTION_CLOSE:
    send_connection_close_ok(connection);
    close_connection(connection);
    break;
  case CONNECTION_CLOSE_OK:
    THROW("Not implemented");
    break;
  default:
    THROW("Not suported");
  }
}

void free_connection(Connection* connection){
  if(connection->is_consumer) {
    free(connection->consumer_tag);
  }
  free(connection);
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

Connection *new_AMQP_Connection(int socket) {
  Connection *connection = Malloc(sizeof(*connection));
  connection->socket = socket;
  connection->consumer_tag_size = 0;
  connection->consumer_tag = NULL;
  connection->is_closed = 0;
  connection->is_consumer = 0;
  init_connection(connection);
  return connection;
}

void init_connection(Connection *connection) {
  receive_protocol_header(connection);
  send_connection_start(connection);
}

void close_connection(Connection *connection) {
  close(connection->socket); 
  connection->is_closed = 1;
}

void make_connection_config(Connection *connection, IMQP_Byte *arguments) {
  void *index = arguments;

  connection->config.channel_max = message_break_s(&index);
  connection->config.frame_max = message_break_l(&index);
  connection->config.heartbeat = message_break_s(&index);
}

/* SEND / RECEIVE */

void receive_protocol_header(Connection *connection) {
  char garbage[10] = {0};
  Read(connection->socket, garbage, MAX_FRAME_SIZE);
}

void send_connection_start(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_START, FAKE_CONNECTION_START_SIZE);
}

void send_connection_tune(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_TUNE, FAKE_CONNECTION_TUNE_SIZE);
}

void send_connection_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_OPEN_OK,
        FAKE_CONNECTION_OPEN_OK_SIZE);
}

void send_connection_close_ok(Connection *connection) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = BASE_COMMUNICATION_CHANNEL;

  uint16_t class = CONNECTION_CLASS;
  uint16_t method = CONNECTION_CLOSE_OK;

  uint32_t payload_size = sizeof(class) + sizeof(method);

  /* + 1 because of frame-end %xCE */
  uint32_t message_size =
      sizeof(type) + sizeof(channel) + sizeof(payload_size) + payload_size + 1;

  uint8_t *message = Malloc(message_size);

  void *index = message;

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &payload_size);
  message_build_s(&index, &class);
  message_build_s(&index, &method);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}
