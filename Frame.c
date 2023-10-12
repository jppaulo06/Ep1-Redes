#include <arpa/inet.h>
#include <string.h>

#include <stdio.h>
#include <unistd.h>

#include "Basic.h"
#include "Channel.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"

/*
 * This file defines the most low level functions, such as deserialize,
 * serialize and send packages stuff.
 */

/*====================================*/
/* EXTERN STUFF */
/*====================================*/

extern const char *FAKE_CONNECTION_START;
extern const int FAKE_CONNECTION_START_SIZE;

extern const char *FAKE_CONNECTION_TUNE;
extern const int FAKE_CONNECTION_TUNE_SIZE;

extern const char *FAKE_CONNECTION_OPEN_OK;
extern const int FAKE_CONNECTION_OPEN_OK_SIZE;

extern const char *FAKE_CHANNEL_OPEN_OK;
extern const int FAKE_CHANNEL_OPEN_OK_SIZE;

extern void define_consumer_tag(Connection *connection);

extern SOAG son_of_a_greve;

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_FRAMES 1000

/* From
 * https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
 */
#if __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) (((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static uint64_t receive_frame(Connection *connection, IMQP_Frame *frame);
static uint64_t receive_frame_header(Connection *connection, IMQP_Frame *frame);
static uint64_t receive_frame_payload(Connection *connection,
                                      IMQP_Frame *frame);
static uint64_t receive_frame_end(Connection *connection);

static uint64_t process_frame(Connection *connection, IMQP_Frame frame);

static uint64_t process_method_frame(Connection *connection,
                                     Method_Payload payload);
static uint64_t process_header_frame(Connection *connection,
                                     Head_Payload payload);
static uint64_t process_body_frame(Connection *connection,
                                   Body_Payload payload);

static uint64_t break_frame_header(IMQP_Byte *message, IMQP_Frame *frame);

/* METHOD FRAMES */
static uint64_t break_method_frame_payload(IMQP_Byte *message,
                                           Method_Payload *frame,
                                           int payload_size);

static uint64_t break_method_frame_arguments(IMQP_Byte *message,
                                             Method_Payload *payload);

/* QUEUE */
static uint64_t break_frame_method_queue_arguments(IMQP_Byte *message,
                                                   Method_Payload *payload);
static uint64_t break_queue_declare_arguments(IMQP_Byte *message,
                                              Queue_Declare *arguments);

/* CONNECTION */
static uint64_t
break_frame_method_connection_arguments(IMQP_Byte *message,
                                        Method_Payload *payload);
static uint64_t
break_connection_tune_ok_arguments(IMQP_Byte *message,
                                   Connection_Tune_Ok *arguments);

/* CHANNEL */
static uint64_t break_frame_method_channel_arguments(IMQP_Byte *message,
                                                     Method_Payload *payload);

/* BASIC */
static uint64_t break_frame_method_basic_arguments(IMQP_Byte *message,
                                                   Method_Payload *payload);
static uint64_t break_basic_consume_arguments(IMQP_Byte *message,
                                              Basic_Consume *arguments);
static uint64_t break_basic_publish_arguments(IMQP_Byte *message,
                                              Basic_Publish *arguments);

static uint64_t break_header_frame_payload(IMQP_Byte *message,
                                           Head_Payload *payload);
static uint64_t break_body_frame_payload(IMQP_Byte *message,
                                         Body_Payload *payload);

/* Build frames */
static int build_queue_declare_ok(IMQP_Byte *message, IMQP_Queue *queue);
static int build_connection_tune(IMQP_Byte *message, Connection *connection);
static int build_connection_start(IMQP_Byte *message, Connection *connection);
static int build_connection_close_ok(IMQP_Byte *message);
static int build_basic_consume_ok(Connection *connection, IMQP_Byte *message);
static int build_basic_deliver_method(Connection *connection,
                                      IMQP_Byte *message, char *queue_name);
static int build_basic_deliver_header(IMQP_Byte *message, uint64_t body_size);
static int build_basic_deliver_body(IMQP_Byte *message, IMQP_Byte *body,
                                    uint32_t body_size);
static int build_channel_close_ok(IMQP_Byte *message);

/* serialization functions */

static uint8_t message_break_b(void **index);
static uint16_t message_break_s(void **index);
static uint32_t message_break_l(void **index);
static uint64_t message_break_ll(void **index);
static void message_break_n(void *content, void **index, uint16_t n);

/* deserialization functions */

static void message_build_b(void **index, uint8_t content);
static void message_build_s(void **index, uint16_t content);
static void message_build_l(void **index, uint32_t content);
static void message_build_ll(void **index, uint64_t content);
static void message_build_n(void **index, void *content, uint16_t n);

/*====================================*/
/* UNIQUE PUBLIC ENTRYPOINT */
/*====================================*/

uint64_t receive_and_process_frame(Connection *connection) {
  uint64_t flags = NO_ERROR;
  if (connection->closed || connection->consumer) {
    return flags;
  }
  IMQP_Frame frame = {0};

  if ((flags |= receive_frame(connection, &frame)) & ERRORS)
    return flags;

  if ((flags |= process_frame(connection, frame)) & ERRORS) {
    printf("Deu pau!\n");
    return flags;
  }

  if (flags) {
    printf("Alguma coisa deu errado... \n");
  }

  return flags | receive_and_process_frame(connection);
}

/*====================================*/
/* PROCESSING FRAMES */
/*====================================*/

uint64_t process_frame(Connection *connection, IMQP_Frame frame) {
  uint64_t flags = 0;
  switch ((enum IMQP_Frame_t)frame.type) {
  case METHOD_FRAME:
    flags |= process_method_frame(connection, frame.payload.method_pl);
    break;
  case HEADER_FRAME:
    flags |= process_header_frame(connection, frame.payload.header_pl);
    break;
  case BODY_FRAME:
    flags |= process_body_frame(connection, frame.payload.body_pl);
    break;
  case HEARTBEAT_FRAME:
    flags |= HEARTBEAT_IGNORED;
    break;
  default:
    flags |= INVALID_FRAME_TYPE;
  }
  return flags;
}

uint64_t process_method_frame(Connection *connection, Method_Payload payload) {
  uint64_t flags = 0;
#ifdef DEBUG_MODE
  printf("[METHOD FRAME] Class: %d - Method: %d\n", payload.class_method,
         payload.method);
#endif /* DEBUG_MODE */
  switch ((enum IMQP_Frame_Class)payload.class_method) {
  case QUEUE_CLASS:
    flags |= process_frame_queue(connection, payload);
    break;
  case CONNECTION_CLASS:
    flags |= process_frame_connection(connection, payload);
    break;
  case CHANNEL_CLASS:
    flags |= process_frame_channel(connection, payload);
    break;
  case BASIC_CLASS:
    flags |= process_frame_basic(connection, payload);
    break;
  default:
    flags |= NOT_EXPECTED_CLASS;
  }
  return flags;
}

uint64_t process_header_frame(Connection *connection, Head_Payload payload) {
  uint64_t flags = 0;
#ifdef DEBUG_MODE
  printf("[HEADER FRAME] Class: %d\n", payload.class_header);
#endif /* DEBUG_MODE */
  switch ((enum IMQP_Frame_Class)payload.class_header) {
  case BASIC_CLASS:
    define_pub_body_size(payload.body_size);
    break;
  default:
    flags |= NOT_EXPECTED_CLASS;
  }
  return flags;
}

uint64_t process_body_frame(Connection *connection, Body_Payload payload) {
  uint64_t flags = 0;
#ifdef DEBUG_MODE
  printf("[BODY FRAME] Body: %s\n", payload.body);
#endif /* DEBUG_MODE */
  define_pub_body(payload.body);
  flags |= finish_pub();
  return flags;
}

/*====================================*/
/* GENERAL FRAME FUNCTIONS */
/*====================================*/

uint64_t receive_frame(Connection *connection, IMQP_Frame *frame) {
  uint64_t flags = 0;
  flags |= receive_frame_header(connection, frame);
  flags |= receive_frame_payload(connection, frame);
  flags |= receive_frame_end(connection);
  return flags;
}

uint64_t receive_frame_header(Connection *connection, IMQP_Frame *frame) {
  uint64_t flags = NO_ERROR;
  IMQP_Byte message[20] = {0};
  recv(connection->socket, message, 7, MSG_WAITALL);
  flags |= break_frame_header(message, frame);
  return flags;
}

uint64_t receive_frame_payload(Connection *connection, IMQP_Frame *frame) {
  uint64_t flags = NO_ERROR;

  IMQP_Byte message[frame->payload_size];
  recv(connection->socket, message, frame->payload_size, MSG_WAITALL);
  IMQP_Payload *payload = &frame->payload;

  switch (frame->type) {
  case METHOD_FRAME:
    flags |= break_method_frame_payload(message, &payload->method_pl,
                                        frame->payload_size);
    break;
  case HEADER_FRAME:
    flags |= break_header_frame_payload(message, &payload->header_pl);
    break;
  case BODY_FRAME:
    flags |= break_body_frame_payload(message, &payload->body_pl);
    break;
  case HEARTBEAT_FRAME:
    flags |= HEARTBEAT_IGNORED;
    break;
  }
  return flags;
}

uint64_t receive_frame_end(Connection *connection) {
  IMQP_Byte frame_end = 0;
  recv(connection->socket, &frame_end, 1, MSG_WAITALL);
  if (frame_end != '\xce') {
    return INVALID_FRAME;
  }
  return NO_ERROR;
}

/*====================================*/
/* BREAKING FRAMES */
/*====================================*/

uint64_t break_frame_header(IMQP_Byte *message, IMQP_Frame *frame) {
  void *offset = message;

  frame->type = message_break_b(&offset);
  if (frame->type >= MAX_FRAME_TYPE)
    return INVALID_FRAME_TYPE;

  frame->channel = message_break_s(&offset);
  if (frame->channel != BASE_COMMUNICATION_CHANNEL &&
      frame->channel != UNIQUE_COMMUNICATION_CHANNEL)
    return INVALID_FRAME_CHANNEL;

  frame->payload_size = message_break_l(&offset);
  return NO_ERROR;
}

/*====================================*/
/* BREAKING METHOD FRAMES */
/*====================================*/

uint64_t receive_protocol_header(Connection *connection) {
  char garbage[8] = {0};
  recv(connection->socket, garbage, 8, MSG_WAITALL);
  return NO_ERROR;
}

uint64_t break_method_frame_payload(IMQP_Byte *message, Method_Payload *payload,
                                    int payload_size) {
  uint64_t flags = NO_ERROR;
  void *offset = message;
  payload->class_method = message_break_s(&offset);
  payload->method = message_break_s(&offset);

  payload->arguments_size = payload_size - (offset - (void *)message);

  flags |= break_method_frame_arguments(offset, payload);
  return flags;
}

uint64_t break_method_frame_arguments(IMQP_Byte *message,
                                      Method_Payload *payload) {
  uint64_t flags = NO_ERROR;
  switch ((enum IMQP_Frame_Class)payload->class_method) {
  case QUEUE_CLASS:
    flags |= break_frame_method_queue_arguments(message, payload);
    break;
  case CONNECTION_CLASS:
    flags |= break_frame_method_connection_arguments(message, payload);
    break;
  case CHANNEL_CLASS:
    flags |= break_frame_method_channel_arguments(message, payload);
    break;
  case BASIC_CLASS:
    flags |= break_frame_method_basic_arguments(message, payload);
    break;
  default:
    flags |= NOT_EXPECTED_CLASS;
    break;
  }
  return flags;
}

/************ QUEUE METHOD ************/

uint64_t break_frame_method_queue_arguments(IMQP_Byte *message,
                                            Method_Payload *payload) {
  uint64_t flags = NO_ERROR;
  switch ((enum IMQP_Frame_Queue)payload->method) {
  case QUEUE_DECLARE:
    flags |= break_queue_declare_arguments(message,
                                           &payload->arguments.queue_declare);
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
    break;
  }
  return flags;
}

uint64_t break_queue_declare_arguments(IMQP_Byte *message,
                                       Queue_Declare *arguments) {
  void *offset = message;
  uint8_t name_size;

  arguments->ticket = message_break_s(&offset);
  name_size = message_break_b(&offset);

  message_break_n(arguments->queue_name, &offset, name_size);
  arguments->queue_name[name_size] = '\x00';

  if (strlen(arguments->queue_name) != name_size) {
    return INVALID_NAME_SIZE;
  }

  return NO_ERROR;
}

/************ CONNECTION METHOD ************/

uint64_t break_frame_method_connection_arguments(IMQP_Byte *message,
                                                 Method_Payload *payload) {
  uint64_t flags = NO_ERROR;
  switch ((enum IMQP_Frame_Connection)payload->method) {
  case CONNECTION_START_OK:
    // Ignore the meta information entirely
    break;
  case CONNECTION_TUNE_OK:
    flags |= break_connection_tune_ok_arguments(
        message, &payload->arguments.connection_tune_ok);
    break;
  case CONNECTION_OPEN:
    // TODO: nothing happens here yet
    break;
  case CONNECTION_CLOSE:
    // TODO: nothing happens here yet
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
    break;
  }
  return flags;
}

uint64_t break_connection_tune_ok_arguments(IMQP_Byte *message,
                                            Connection_Tune_Ok *arguments) {
  // TODO: verify the content from here if it matches with my connection TUNE
  void *index = arguments;

  arguments->channel_max = message_break_s(&index);
  arguments->frame_max = message_break_l(&index);
  arguments->heartbeat = message_break_s(&index);
  return NO_ERROR;
}

/************ CHANNEL METHOD ************/

uint64_t break_frame_method_channel_arguments(IMQP_Byte *message,
                                              Method_Payload *payload) {
  uint64_t flags = NO_ERROR;
  switch ((enum IMQP_Frame_Channel)payload->method) {
  case CHANNEL_OPEN:
    // TODO: nothing happens here yet
    break;
  case CHANNEL_CLOSE:
    // TODO: nothing happens here yet
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
    break;
  }
  return flags;
}

/************ BASIC METHOD ************/

uint64_t break_frame_method_basic_arguments(IMQP_Byte *message,
                                            Method_Payload *payload) {
  uint64_t flags = NO_ERROR;
  switch ((enum IMQP_Frame_Basic)payload->method) {
  case BASIC_CONSUME:
    flags |= break_basic_consume_arguments(message,
                                           &payload->arguments.basic_consume);
    break;
  case BASIC_PUBLISH:
    flags |= break_basic_publish_arguments(message,
                                           &payload->arguments.basic_publish);
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
    break;
  }
  return flags;
}

uint64_t break_basic_consume_arguments(IMQP_Byte *message,
                                       Basic_Consume *arguments) {
  void *offset = message;
  uint8_t name_size;
  uint8_t ctag_size;

  arguments->ticket = message_break_s(&offset);

  name_size = message_break_b(&offset);
  message_break_n(arguments->queue_name, &offset, name_size);
  arguments->queue_name[name_size] = '\x00';

  if (strlen(arguments->queue_name) != name_size) {
    return INVALID_NAME_SIZE;
  }

  ctag_size = message_break_b(&offset);
  if (ctag_size > CONSUMER_TAG_SIZE) {
    return INVALID_CONSUMER_TAG_SIZE;
  }
  message_break_n(arguments->consumer_tag, &offset, ctag_size);

  arguments->config_flags = message_break_b(&offset);
  return NO_ERROR;
}

uint64_t break_basic_publish_arguments(IMQP_Byte *message,
                                       Basic_Publish *arguments) {
  void *offset = message;
  uint8_t name_size;
  uint8_t exchange_size;

  arguments->ticket = message_break_s(&offset);

  exchange_size = message_break_b(&offset);
  message_break_n(arguments->exchange_name, &offset, exchange_size);
  arguments->exchange_name[exchange_size] = '\x00';

  if (strlen(arguments->exchange_name) != exchange_size) {
    return INVALID_EXCHANGE_NAME_SIZE;
  }

  name_size = message_break_b(&offset);
  message_break_n(arguments->queue_name, &offset, name_size);
  arguments->queue_name[name_size] = '\x00';

  if (strlen(arguments->queue_name) != name_size) {
    return INVALID_NAME_SIZE;
  }

  arguments->config_flags = message_break_b(&offset);
  return NO_ERROR;
}

/*====================================*/
/* BREAKING HEADER FRAMES */
/*====================================*/

uint64_t break_header_frame_payload(IMQP_Byte *message, Head_Payload *payload) {
  void *offset = message;
  payload->class_header = message_break_s(&offset);
  payload->weight = message_break_s(&offset);
  payload->body_size = message_break_ll(&offset);
  return NO_ERROR;
}

/*====================================*/
/* SEND FRAMES */
/*====================================*/

/************ QUEUE METHOD ************/

void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue) {
  IMQP_Byte message[MAX_FRAME_SIZE];
  int message_size = 0;
  message_size += build_queue_declare_ok(message, queue);
  Write(connection->socket, (const char *)message, message_size);
}

/************ CONNECTION METHOD ************/

void send_connection_start(Connection *connection) {
  IMQP_Byte message[MAX_FRAME_SIZE];
  int message_size = 0;
  message_size += build_connection_start(message, connection);
  Write(connection->socket, (const char *)message, message_size);
}

void send_connection_tune(Connection *connection) {
  IMQP_Byte message[MAX_FRAME_SIZE];
  int message_size = 0;
  message_size += build_connection_tune(message, connection);
  Write(connection->socket, (const char *)message, message_size);
}

void send_connection_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CONNECTION_OPEN_OK,
        FAKE_CONNECTION_OPEN_OK_SIZE);
}

void send_connection_close_ok(Connection *connection) {
  IMQP_Byte message[MAX_FRAME_SIZE];
  int message_size = 0;
  message_size += build_connection_close_ok(message);
  Write(connection->socket, (const char *)message, message_size);
}

/************ BASIC METHOD ************/

void send_basic_deliver(Connection *connection, char *queue_name,
                        IMQP_Byte *body, uint32_t body_size) {

  IMQP_Byte message[3 * MAX_FRAME_SIZE];

  int message_size = 0;
  void *offset = message;

  message_size += build_basic_deliver_method(connection, offset, queue_name);

  offset = message + message_size;
  message_size += build_basic_deliver_header(offset, body_size);

  offset = message + message_size;
  message_size += build_basic_deliver_body(offset, body, body_size);

  Write(connection->socket, message, message_size);
}

void send_basic_consume_ok(Connection *connection) {

  IMQP_Byte message[MAX_FRAME_SIZE];
  int message_size = 0;

  define_consumer_tag(connection);
  message_size += build_basic_consume_ok(connection, message);

  Write(connection->socket, (const char *)message, message_size);
}

/************ CHANNEL METHOD ************/

// TODO: REMOVE THIS MOCK!!
void send_channel_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CHANNEL_OPEN_OK, FAKE_CHANNEL_OPEN_OK_SIZE);
}

void send_channel_close_ok(Connection *connection) {
  IMQP_Byte message[MAX_FRAME_SIZE];

  int message_size = 0;

  message_size += build_channel_close_ok(message);

  Write(connection->socket, (const char *)message, message_size);
}

/*====================================*/
/* BUILD FRAMES */
/*====================================*/

/************ QUEUE METHOD ************/

int build_queue_declare_ok(IMQP_Byte *message, IMQP_Queue *queue) {

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

  void *offset = message;

  message_build_b(&offset, type);
  message_build_s(&offset, channel);
  message_build_l(&offset, payload_size);
  message_build_s(&offset, class);
  message_build_s(&offset, method);
  message_build_b(&offset, queue->name_size);
  message_build_n(&offset, queue->name, queue->name_size);
  message_build_l(&offset, message_count);
  message_build_l(&offset, consumer_count);
  message_build_b(&offset, FRAME_END);

  return offset - (void *)message;
}

/************ CONNECTION METHOD ************/

int build_connection_tune(IMQP_Byte *message, Connection *connection) {

  uint8_t type = METHOD_FRAME;
  uint16_t channel = BASE_COMMUNICATION_CHANNEL;

  uint16_t class = CONNECTION_CLASS;
  uint16_t method = CONNECTION_TUNE;

  const Config *config = &(son_of_a_greve.config);

  /* + 1 because of name size */
  uint32_t payload_size = sizeof(class) + sizeof(method) + sizeof(config->channel_max) +
    sizeof(config->frame_max) + sizeof(config->heartbeat);
                          
  void *offset = message;

  message_build_b(&offset, type);
  message_build_s(&offset, channel);
  message_build_l(&offset, payload_size);

  message_build_s(&offset, class);
  message_build_s(&offset, method);

  message_build_s(&offset, config->channel_max);
  message_build_l(&offset, config->frame_max);
  message_build_s(&offset, config->heartbeat);

  message_build_b(&offset, FRAME_END);

  return offset - (void *)message;
}

int build_connection_start(IMQP_Byte *message, Connection *connection) {

  uint8_t type = METHOD_FRAME;
  uint16_t channel = BASE_COMMUNICATION_CHANNEL;

  uint16_t class = CONNECTION_CLASS;
  uint16_t method = CONNECTION_START;

  const char *str_cluster_name = "cluster_name";
  const char *str_license = "license";
  const char *str_product = "product";

  const Meta *meta = &(son_of_a_greve.meta);

  uint32_t properties_size = 1 + strlen(str_cluster_name) + 1 + 4 + strlen(meta->cluster_name) +
      1 + strlen(str_license) + 1 + 4 + strlen(meta->license) +
      1 + strlen(str_product) + 1 + 4 + strlen(meta->product);

  uint32_t arguments_size =
      sizeof(meta->major_version) + sizeof(meta->major_version) + 4 +
      properties_size +
      4 + strlen(meta->mechanisms) +
      4 + strlen(meta->locales);

  /* + 1 because of name size */
  uint32_t payload_size = sizeof(class) + sizeof(method) + arguments_size;
                          
  void *offset = message;

  message_build_b(&offset, type);
  message_build_s(&offset, channel);
  message_build_l(&offset, payload_size);

  message_build_s(&offset, class);
  message_build_s(&offset, method);

  message_build_b(&offset, meta->major_version);
  message_build_b(&offset, meta->minor_version);

  message_build_l(&offset, properties_size);

  message_build_b(&offset, strlen(str_cluster_name));
  message_build_n(&offset, (void*)str_cluster_name, strlen(str_cluster_name));
  message_build_b(&offset, 'S');
  message_build_l(&offset, strlen(meta->cluster_name));
  message_build_n(&offset, (void*)meta->cluster_name, strlen(meta->cluster_name));

  message_build_b(&offset, strlen(str_license));
  message_build_n(&offset, (void*)str_license, strlen(str_license));
  message_build_b(&offset, 'S');
  message_build_l(&offset, strlen(meta->license));
  message_build_n(&offset, (void*)meta->license, strlen(meta->license));

  message_build_b(&offset, strlen(str_product));
  message_build_n(&offset, (void*)str_product, strlen(str_product));
  message_build_b(&offset, 'S');
  message_build_l(&offset, strlen(meta->product));
  message_build_n(&offset, (void*)meta->product, strlen(meta->product));

  message_build_l(&offset, strlen(meta->mechanisms));
  message_build_n(&offset, (void*)meta->mechanisms, strlen(meta->mechanisms));

  message_build_l(&offset, strlen(meta->locales));
  message_build_n(&offset, (void*)meta->locales, strlen(meta->locales));

  message_build_b(&offset, FRAME_END);

  return offset - (void *)message;
}

int build_connection_close_ok(IMQP_Byte *message) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = BASE_COMMUNICATION_CHANNEL;

  uint16_t class = CONNECTION_CLASS;
  uint16_t method = CONNECTION_CLOSE_OK;

  uint32_t payload_size = sizeof(class) + sizeof(method);

  void *index = message;

  message_build_b(&index, type);
  message_build_s(&index, channel);
  message_build_l(&index, payload_size);
  message_build_s(&index, class);
  message_build_s(&index, method);
  message_build_b(&index, FRAME_END);

  return index - (void *)message;
}

/************ BASIC METHOD ************/

int build_basic_consume_ok(Connection *connection, IMQP_Byte *message) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;
  uint16_t class = BASIC_CLASS;
  uint16_t method = BASIC_CONSUME_OK;

  /* + 1 because of size of consumer tag */
  uint32_t payload_size =
      sizeof(class) + sizeof(method) + connection->consumer_tag_size + 1;

  void *offset = message;

  message_build_b(&offset, type);
  message_build_s(&offset, channel);
  message_build_l(&offset, payload_size);
  message_build_s(&offset, class);
  message_build_s(&offset, method);
  message_build_b(&offset, connection->consumer_tag_size);
  message_build_n(&offset, (void *)connection->consumer_tag,
                  connection->consumer_tag_size);
  message_build_b(&offset, FRAME_END);

  return offset - (void *)message;
}

int build_basic_deliver_method(Connection *connection, IMQP_Byte *message,
                               char *queue_name) {

  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;
  uint16_t class = BASIC_CLASS;
  uint16_t method = BASIC_DELIVER;
  const char *mocks = "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00";
  const int mocks_size = 10;

  uint8_t queue_name_size = strlen(queue_name);

  /* +1 because size of consumer tag && + 1 because of queue_name_size*/
  uint32_t arguments_size =
      1 + connection->consumer_tag_size + mocks_size + 1 + strlen(queue_name);

  uint32_t payload_size = sizeof(class) + sizeof(method) + arguments_size;

  void *index = message;

  message_build_b(&index, type);
  message_build_s(&index, channel);
  message_build_l(&index, payload_size);
  message_build_s(&index, class);
  message_build_s(&index, method);
  message_build_b(&index, connection->consumer_tag_size);
  message_build_n(&index, (void *)connection->consumer_tag,
                  connection->consumer_tag_size);
  message_build_n(&index, (void *)mocks, mocks_size);
  message_build_b(&index, queue_name_size);
  message_build_n(&index, (void *)queue_name, queue_name_size);
  message_build_b(&index, FRAME_END);

  return index - (void *)message;
}

int build_basic_deliver_header(IMQP_Byte *message, uint64_t body_size) {

  uint8_t type = HEADER_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;
  uint16_t class = BASIC_CLASS;
  uint16_t weight = 0;
  const char *mocks = "\x10\x00\x01";
  const int mocks_size = 3;
  uint32_t payload_size =
      sizeof(class) + sizeof(weight) + sizeof(body_size) + mocks_size;
  void *index = message;

  message_build_b(&index, type);
  message_build_s(&index, channel);
  message_build_l(&index, payload_size);
  message_build_s(&index, class);
  message_build_s(&index, weight);
  message_build_ll(&index, body_size);
  message_build_n(&index, (void *)mocks, mocks_size);
  message_build_b(&index, FRAME_END);

  return index - (void *)message;
}

int build_basic_deliver_body(IMQP_Byte *message, IMQP_Byte *body,
                             uint32_t body_size) {

  uint8_t type = BODY_FRAME;

  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  void *index = message;

  int size = body_size;

  message_build_b(&index, type);
  message_build_s(&index, channel);
  message_build_l(&index, body_size);
  message_build_n(&index, body, size);
  message_build_b(&index, FRAME_END);

  return index - (void *)message;
}

/************ CHANNEL METHOD ************/

int build_channel_close_ok(IMQP_Byte *message) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = CHANNEL_CLASS;
  uint16_t method = CHANNEL_CLOSE_OK;

  uint32_t payload_size = sizeof(class) + sizeof(method);

  void *offset = message;

  message_build_b(&offset, type);
  message_build_s(&offset, channel);
  message_build_l(&offset, payload_size);
  message_build_s(&offset, class);
  message_build_s(&offset, method);
  message_build_b(&offset, FRAME_END);

  return offset - (void *)message;
}

/*====================================*/
/* BREAKING BODY FRAMES */
/*====================================*/

uint64_t break_body_frame_payload(IMQP_Byte *message, Body_Payload *payload) {
  void *offset = message;
  int pub_size = get_pub_size();
  message_break_n(payload->body, &offset, pub_size);
  payload->body[pub_size] = '\x00';
  return NO_ERROR;
}
/*====================================*/
/* DESERIALIZATION HELPERS */
/*====================================*/

uint8_t message_break_b(void **index) {
  uint8_t content;
  memcpy(&content, *index, sizeof(content));
  *index += sizeof(content);
  return content;
}

uint16_t message_break_s(void **index) {
  uint16_t content;
  memcpy(&content, *index, sizeof(content));
  content = ntohs(content);
  *index += sizeof(content);
  return content;
}

uint32_t message_break_l(void **index) {
  uint32_t content;
  memcpy(&content, *index, sizeof(content));
  content = ntohl(content);
  *index += sizeof(content);
  return content;
}

uint64_t message_break_ll(void **index) {
  uint64_t content;
  memcpy(&content, *index, sizeof(content));
  content = ntohll(content);
  *index += sizeof(content);
  return content;
}

void message_break_n(void *content, void **index, uint16_t n) {
  memcpy(content, *index, n);
  *index += n;
}

/*====================================*/
/* SERIALIZATION HELPERS */
/*====================================*/

void message_build_b(void **index, uint8_t content) {
  memcpy(*index, &content, sizeof(content));
  *index += sizeof(content);
}

void message_build_s(void **index, uint16_t content) {
  content = htons(content);
  memcpy(*index, &content, sizeof(content));
  *index += sizeof(content);
}

void message_build_l(void **index, uint32_t content) {
  content = htonl(content);
  memcpy(*index, &content, sizeof(content));
  *index += sizeof(content);
}

void message_build_ll(void **index, uint64_t content) {
  content = htonll(content);
  memcpy(*index, &content, sizeof(content));
  *index += sizeof(content);
}

void message_build_n(void **index, void *content, uint16_t n) {
  memcpy(*index, content, n);
  *index += n;
}
