#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "imqp.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define MAX_FRAME_SIZE 3000
#define INITIAL_MAX_CONNECTIONS 1000
#define BASE_COMMUNICATION_CHANNEL 0
#define UNIQUE_COMMUNICATION_CHANNEL 1

#define FRAME_END ((uint8_t *)"\xCE")

/*====================================*/
/* EXTERNAL VARIABLES */
/*====================================*/

extern const char *FAKE_CONNECTION_START;
extern const int FAKE_CONNECTION_START_SIZE;

extern const char *FAKE_CONNECTION_TUNE;
extern const int FAKE_CONNECTION_TUNE_SIZE;

extern const char *FAKE_CONNECTION_OPEN_OK;
extern const int FAKE_CONNECTION_OPEN_OK_SIZE;

extern const char *FAKE_CHANNEL_OPEN_OK;
extern const int FAKE_CHANNEL_OPEN_OK_SIZE;

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

/* Init connection functions */

static void init_connection(Connection *connection);
static int connection_is_off(Connection *connection);

static void receive_protocol_header(Connection *connection);

static void send_connection_start(Connection *connection);

static void send_connection_tune(Connection *connection);

static void send_connection_open_ok(Connection *connection);

static void send_channel_open_ok(Connection *connection);

/* General frame functions */

static void throw_frame_away(Connection *connection);

static IMQP_Frame *new_IMQP_Frame();

static void receive_frame(Connection *connection, IMQP_Frame *frame);

static void receive_frame_header(Connection *connection, IMQP_Frame *frame);
static void receive_frame_arguments(Connection *connection, IMQP_Frame *frame);
static void receive_frame_end(Connection *connection);

static void release_frame(IMQP_Frame **frame);

/* Deserialization functions */

static void message_break_b(uint8_t *content, void **index);
static void message_break_s(uint16_t *content, void **index);
static void message_break_l(uint32_t *content, void **index);
static void message_break_n(void *content, void **index, uint16_t n);

/* Serialization functions */

static void message_build_b(void **index, uint8_t *content);
static void message_build_s(void **index, uint16_t *content);
static void message_build_l(void **index, uint32_t *content);
static void message_build_n(void **index, void *content, uint16_t n);

/* Connection true processing */

static void process_method_frame(Connection *connection, IMQP_Frame *frame);

/* Queue */
static void process_frame_queue(Connection *connection, IMQP_Frame *frame);
static IMQP_Queue *new_IMQP_Queue(IMQP_Argument *argument);
static void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue);

/* Connection */
static void process_frame_connection(Connection *connection, IMQP_Frame *frame);
static void send_connection_close_ok(Connection *connection);
static void make_connection_config(Connection* connection, IMQP_Argument *arguments);

/* Channel */
static void process_frame_channel(Connection *connection, IMQP_Frame *frame);
static void send_channel_close_ok(Connection *connection);

static void close_connection(Connection *connection);

/*====================================*/
/* INIT CONNECTION FUNCTIONS */
/*====================================*/

Connection *new_AMQP_Connection(int socket) {
  Connection *connection = Malloc(sizeof(*connection));
  connection->socket = socket;
  printf("socket: %d\n", connection->socket);
  init_connection(connection);
  return connection;
}

void init_connection(Connection *connection) {
  receive_protocol_header(connection);

  printf("socket: %d\n", connection->socket);

  send_connection_start(connection);
}

void receive_protocol_header(Connection *connection) {
  char garbage[10] = {0};
  Read(connection->socket, garbage, MAX_FRAME_SIZE);
}

/* CONNECTION START */
void send_connection_start(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_START, FAKE_CONNECTION_START_SIZE);
}

/* CONNECTION TUNE */
void send_connection_tune(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_TUNE, FAKE_CONNECTION_TUNE_SIZE);
}

/* CONNECTION OPEN */
void send_connection_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_OPEN_OK,
        FAKE_CONNECTION_OPEN_OK_SIZE);
}

/* CHANNEL OPEN */
void send_channel_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CHANNEL_OPEN_OK, FAKE_CHANNEL_OPEN_OK_SIZE);
}

/*====================================*/
/* GENERAL FRAME FUNCTIONS */
/*====================================*/

void throw_frame_away(Connection *connection) {
  IMQP_Frame *frame = new_IMQP_Frame();
  receive_frame(connection, frame);
  release_frame(&frame);
}

IMQP_Frame *new_IMQP_Frame() {
  IMQP_Frame *frame = Malloc(sizeof(IMQP_Frame *));
  bzero(frame, sizeof(*frame));
  return frame;
}

void receive_frame(Connection *connection, IMQP_Frame *frame) {
  receive_frame_header(connection, frame);
  receive_frame_arguments(connection, frame);
  receive_frame_end(connection);
}

void receive_frame_header(Connection *connection, IMQP_Frame *frame) {

  char buffer[20];
  uint32_t payload_size, extra_mem = 0;

  read(connection->socket, buffer, 11);

  void *index = buffer;

  /* 1 byte only => no need for network to host conversion */
  memcpy(&frame->type, index, sizeof(frame->type));
  index += sizeof(frame->type);

  memcpy(&frame->channel, index, sizeof(frame->channel));
  frame->channel = ntohs(frame->channel);
  index += sizeof(frame->channel);

  memcpy(&payload_size, index, sizeof(frame->arguments_size));
  payload_size = ntohl(payload_size);
  index += sizeof(payload_size);

  switch ((enum IMQP_Frame_t)frame->type) {
  case METHOD_FRAME:
    extra_mem = sizeof(frame->class_method) + sizeof(frame->method);

    memcpy(&frame->class_method, index, sizeof(frame->class_method));
    frame->class_method = ntohs(frame->class_method);
    index += sizeof(frame->class_method);

    memcpy(&frame->method, index, sizeof(frame->method));
    frame->method = ntohs(frame->method);
    break;

  case HEADER_FRAME:
    THROW("Not implemented method");
    break;

  case BODY_FRAME:
    THROW("Not implemented method");
    break;

  case HEARTBEAT_FRAME:
    THROW("Not implemented method");
    break;
  }

  frame->arguments_size = payload_size - extra_mem;

  printf("arguments size: %d\n", frame->arguments_size);
}

void receive_frame_arguments(Connection *connection, IMQP_Frame *frame) {
  if (frame->arguments_size > 0) {
    frame->arguments = Malloc(frame->arguments_size);
    read(connection->socket, frame->arguments, frame->arguments_size);
  }
}

void receive_frame_end(Connection *connection) {
  char garbage;
  read(connection->socket, &garbage, 1);
}

void release_frame(IMQP_Frame **frame) {
  free((*frame)->arguments);
  free(*frame);
  *frame = NULL;
}

/* DESERIALIZATION FUNCTIONS */

void message_break_b(uint8_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *index += sizeof(*content);
}

void message_break_s(uint16_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *content = ntohs(*content);
  *index += sizeof(*content);
}

void message_break_l(uint32_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *content = ntohs(*content);
  *index += sizeof(*content);
}

void message_break_n(void *content, void **index, uint16_t n) {
  memcpy(content, *index, n);
  *index += n;
}

/* SERIALIZATION FUNCTIONS */

void message_build_b(void **index, uint8_t *content) {
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_s(void **index, uint16_t *content) {
  *content = htons(*content);
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_l(void **index, uint32_t *content) {
  *content = htonl(*content);
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_n(void **index, void *content, uint16_t n) {
  memcpy(*index, content, n);
  *index += n;
}

int connection_is_off(Connection *connection) {
  int error_code;
  int error_code_size = sizeof(error_code);
  if (getsockopt(connection->socket, SOL_SOCKET, SO_ERROR, &error_code,
                 (socklen_t *)&error_code_size) < 0) {
    return 1;
  }
  return 0;
}

void close_connection(Connection *connection) { close(connection->socket); }

/*====================================*/
/* CONNECTION TRUE PROCESSING */
/*====================================*/

void process_action(Connection *connection) {

  if (connection_is_off(connection)) {
    return;
  }

  IMQP_Frame *frame = new_IMQP_Frame();
  receive_frame(connection, frame);
  switch ((enum IMQP_Frame_t)frame->type) {
  case METHOD_FRAME:
    process_method_frame(connection, frame);
    break;
  case HEADER_FRAME:
    THROW("Not implemented method");
    break;
  case BODY_FRAME:
    THROW("Not implemented method");
    break;
  case HEARTBEAT_FRAME:
    THROW("Not implemented method");
    break;
  }
  release_frame(&frame);
  process_action(connection);
}

void process_method_frame(Connection *connection, IMQP_Frame *frame) {
  switch ((enum IMQP_Frame_Method_Class)frame->class_method) {
  case QUEUE_CLASS:
    process_frame_queue(connection, frame);
    break;
  case CONNECTION_CLASS:
    process_frame_connection(connection, frame);
    break;
  case CHANNEL_CLASS:
    process_frame_channel(connection, frame);
    break;
  default:
    THROW("Not suported class");
  }
}

void process_frame_queue(Connection *connection, IMQP_Frame *frame) {
  IMQP_Argument *arguments = frame->arguments;
  IMQP_Queue *queue = NULL;
  switch ((enum IMQP_Frame_Queue)frame->method) {
  case QUEUE_DECLARE:
    queue = new_IMQP_Queue(arguments);
    // add_to_queue_list(queue);
    send_queue_declare_ok(connection, queue);
    break;
  case QUEUE_DECLARE_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

IMQP_Queue *new_IMQP_Queue(IMQP_Argument *arguments) {
  IMQP_Queue *queue = Malloc(sizeof(*queue));

  void *index = arguments;

  message_break_s(&queue->ticket, &index);
  message_break_b(&queue->name_size, &index);

  queue->name = Malloc(queue->name_size + 1);

  message_break_n(queue->name, &index, queue->name_size);
  queue->name[queue->name_size] = '\x00';

  queue->connections = Malloc(sizeof(Connection) * INITIAL_MAX_CONNECTIONS);

  queue->max_connections = INITIAL_MAX_CONNECTIONS;
  queue->total_connections = 0;
  queue->round_robin = 0;

  return queue;
}

void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue) {

  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = QUEUE_CLASS;
  uint16_t method = QUEUE_DECLARE_OK;
  uint32_t message_count = 0;
  uint32_t consumer_count = 0;

  /* + 1 because of name size */
  uint32_t payload_size = sizeof(class) + sizeof(method) +
                          (1 + queue->name_size) + sizeof(message_count) +
                          sizeof(consumer_count);

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
  message_build_b(&index, &queue->name_size);
  message_build_n(&index, queue->name, queue->name_size);
  message_build_l(&index, &message_count);
  message_build_l(&index, &consumer_count);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}

void process_frame_connection(Connection *connection, IMQP_Frame *frame) {
  IMQP_Argument *arguments = frame->arguments;
  switch ((enum IMQP_Frame_Connection)frame->method) {
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

void make_connection_config(Connection* connection, IMQP_Argument *arguments) {

  void *index = arguments;

  message_break_s(&connection->config.channel_max, &index);
  message_break_l(&connection->config.frame_max, &index);
  message_break_s(&connection->config.heartbeat, &index);
}

void process_frame_channel(Connection *connection, IMQP_Frame *frame) {
  // IMQP_Argument *arguments = frame->arguments;
  switch ((enum IMQP_Frame_Channel)frame->method) {
  case CHANNEL_OPEN:
    send_channel_open_ok(connection);
    break;
  case CHANNEL_OPEN_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case CHANNEL_CLOSE:
    send_channel_close_ok(connection);
    break;
  case CHANNEL_CLOSE_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

void send_channel_close_ok(Connection *connection) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = CHANNEL_CLASS;
  uint16_t method = CHANNEL_CLOSE_OK;

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
