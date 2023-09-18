#include <arpa/inet.h>
#include <string.h>

#include "Frame.h"
#include "Connection.h"
#include "Queue.h"
#include "Channel.h"
#include "Basic.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_FRAMES 1000

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static IMQP_Frame *new_IMQP_Frame();
static void throw_frame_away(Connection *connection);
static void receive_frame(Connection *connection, IMQP_Frame *frame);

static void receive_frame_header(Connection *connection, IMQP_Frame *frame);
static void receive_frame_arguments(Connection *connection, IMQP_Frame *frame);
static void receive_frame_end(Connection *connection);

static void release_frame(IMQP_Frame **frame);

static void process_method_frame(Connection *connection, IMQP_Frame *frame);

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_frame(Connection *connection) {
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
  process_frame(connection);
}


/* Deserialization functions */

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

/* Serialization functions */

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

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

IMQP_Frame *new_IMQP_Frame() {
  IMQP_Frame *frame = Malloc(sizeof(IMQP_Frame *));
  bzero(frame, sizeof(*frame));
  return frame;
}

void throw_frame_away(Connection *connection) {
  IMQP_Frame *frame = new_IMQP_Frame();
  receive_frame(connection, frame);
  release_frame(&frame);
}

void receive_frame(Connection *connection, IMQP_Frame *frame) {
  receive_frame_header(connection, frame);
  receive_frame_arguments(connection, frame);
  receive_frame_end(connection);
}

void receive_frame_header(Connection *connection, IMQP_Frame *frame) {

  char buffer[20] = {0};
  uint32_t payload_size, extra_mem = 0;

  Read(connection->socket, buffer, 11);

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
}

void receive_frame_arguments(Connection *connection, IMQP_Frame *frame) {
  if (frame->arguments_size > 0) {
    frame->arguments = Malloc(frame->arguments_size);
    Read(connection->socket, frame->arguments, frame->arguments_size);
  }
}

void receive_frame_end(Connection *connection) {
  char garbage = 0;
  Read(connection->socket, &garbage, 1);
}

void release_frame(IMQP_Frame **frame) {
  free((*frame)->arguments);
  free(*frame);
  *frame = NULL;
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
  case BASIC_CLASS:
    process_frame_basic(connection, frame);
    break;
  default:
    THROW("Not suported class");
  }
}
