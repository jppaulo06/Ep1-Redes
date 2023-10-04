#include "Basic.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Basic {
  BASIC_CONSUME = 20,
  BASIC_CONSUME_OK,
  BASIC_PUBLISH = 40,
  BASIC_DELIVER = 60,
};

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

static void create_new_consumer(Connection *connection, IMQP_Byte *arguments);
static void send_basic_consume_ok(Connection *connection);
static char *create_consumer_tag(uint8_t *size);
static void publish();
static void define_pub_queue_name(IMQP_Byte *arguments);
static void free_pub();
static int build_basic_deliver_method(Connection *connection,
                                      IMQP_Byte *message, char *queue_name);
static int build_basic_deliver_header(IMQP_Byte *message, uint64_t body_size);
static int build_basic_deliver_body(IMQP_Byte *message, IMQP_Byte *body,
                                    uint32_t body_size);

static void *waitForFIN(void *con);
/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

struct {
  uint64_t size;
  char *queue_name;
  IMQP_Byte *body;
} publication;

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_basic(Connection *connection, IMQP_Frame *frame) {
  IMQP_Byte *arguments = frame->payload.method_pl.arguments;
  switch ((enum IMQP_Frame_Basic)frame->payload.method_pl.method) {
  case BASIC_CONSUME:
    create_new_consumer(connection, arguments);
    send_basic_consume_ok(connection);

    pthread_t tid;
    pthread_create(&tid, NULL, waitForFIN, connection);

    connection->is_consumer = 1;

    break;
  case BASIC_CONSUME_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case BASIC_PUBLISH:
    define_pub_queue_name(arguments);
    break;
  default:
    THROW("Not suported");
  }
}

void define_pub_body_size(IMQP_Byte *remaining_payload) {
  void *index = remaining_payload;

  uint16_t garbage;
  uint64_t size;

  message_break_s(&garbage, &index);
  message_break_ll(&size, &index);

  publication.size = size;
  printf("Size: %ju", publication.size);
}

void define_pub_body(IMQP_Byte *payload) {
  void *index = payload;
  IMQP_Byte *body = Malloc(publication.size + 1);

  message_break_n(body, &index, publication.size);

  body[publication.size] = '\x00';

  publication.body = body;
}

void finish_pub() {
  publish();
  free_pub();
}

int get_pub_size() { return publication.size; }

void send_basic_deliver(Connection *connection, char *queue_name,
                        IMQP_Byte *body, uint32_t body_size) {

  IMQP_Byte *message = Malloc(2000);

  int message_size = 0;
  void *index = message;

  message_size += build_basic_deliver_method(connection, index, queue_name);

  index = message + message_size;
  message_size += build_basic_deliver_header(index, body_size);

  index = message + message_size;
  message_size += build_basic_deliver_body(index, body, body_size);

  Write(connection->socket, message, message_size);
  free(message);
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

void create_new_consumer(Connection *connection, IMQP_Byte *arguments) {
  void *index = arguments;

  uint16_t garbage;
  uint8_t name_size;

  message_break_s(&garbage, &index);
  message_break_b(&name_size, &index);

  char *queue_name = Malloc(name_size + 1);

  message_break_n(queue_name, &index, name_size);
  queue_name[name_size] = '\x00';

  put_into_queue(connection, queue_name);
  free(queue_name);
}

void send_basic_consume_ok(Connection *connection) {

  uint8_t type = METHOD_FRAME;

  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = BASIC_CLASS;
  uint16_t method = BASIC_CONSUME_OK;

  uint8_t consumer_tag_size = 20;
  const char *consumer_tag = create_consumer_tag(&consumer_tag_size);

  connection->consumer_tag = (IMQP_Byte *)consumer_tag;
  connection->consumer_tag_size = consumer_tag_size;

  /* + 1 because of size of consumer tag */
  uint32_t payload_size =
      sizeof(class) + sizeof(method) + consumer_tag_size + 1;

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
  message_build_b(&index, &consumer_tag_size);
  message_build_n(&index, (void *)consumer_tag, consumer_tag_size);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}

void define_pub_queue_name(IMQP_Byte *arguments) {
  void *index = arguments;

  uint16_t garbage;
  uint8_t garbage2;
  uint8_t name_size;

  message_break_s(&garbage, &index);
  message_break_b(&garbage2, &index);
  message_break_b(&name_size, &index);

  char *queue_name = Malloc(name_size + 1);

  message_break_n(queue_name, &index, name_size);
  queue_name[name_size] = '\x00';

  printf("BLA queue_name: %s\n", queue_name);

  publication.queue_name = queue_name;
}

void free_pub() {
  free(publication.queue_name);
  free(publication.body);
}

void publish() {
  publish_to_queue(publication.queue_name, publication.body, publication.size);
}

char *create_consumer_tag(uint8_t *size) {
  const char *ctag_default = "amq.ctag-";
  const int ctag_min_len = strlen(ctag_default);
  assert(*size > ctag_min_len && "Input size MUST be greater tan ctag_min_len");

  *size = *size + ctag_min_len;
  char *consumer_tag = Malloc(*size);
  strcpy(consumer_tag, ctag_default);
  for (int i = ctag_min_len; i < *size; i++) {
    consumer_tag[i] = (rand() % ('Z' - 'A')) + 'A';
  }
  return consumer_tag;
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

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &payload_size);
  message_build_s(&index, &class);
  message_build_s(&index, &method);

  message_build_b(&index, &connection->consumer_tag_size);
  message_build_n(&index, (void *)connection->consumer_tag,
                  connection->consumer_tag_size);
  message_build_n(&index, (void *)mocks, mocks_size);
  message_build_b(&index, &queue_name_size);
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

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &payload_size);
  message_build_s(&index, &class);
  message_build_s(&index, &weight);
  message_build_ll(&index, &body_size);
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

  printf("body_size: %d\n", body_size);
  printf("body: %s\n", body);

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &body_size);
  message_build_n(&index, body, size);
  message_build_b(&index, FRAME_END);

  printf("index: %p\n", index);

  return index - (void *)message;
}

void *waitForFIN(void *con) {
  Connection *connection = (Connection *)con;
  int socketfd = connection->socket;
  char buffer[2000];
  int ret = read(socketfd, buffer, 1000);
  if(ret > 0) {
    return waitForFIN(connection);
  }
  close_connection(connection);
  return NULL;
}
