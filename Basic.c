#include "Basic.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define CTAG_DEFAULT_RANDOM_SIZE 20

/*====================================*/
/* GLOBAL FUNCTIONS */
/*====================================*/

void define_consumer_tag(Connection *connection);

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

static void process_basic_consume(Connection *connection,
                                  Basic_Consume arguments);
static void process_basic_publish(Connection *connection,
                                  Basic_Publish arguments);

static void publish();
static void define_pub_queue_name(IMQP_Byte *arguments);

static void *wait_for_FIN(void *fd);

/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

struct {
  uint64_t size;
  char queue_name[MAX_QUEUE_NAME_SIZE];
  IMQP_Byte body[MAX_BODY_SIZE];
} publication;

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_basic(Connection *connection, Method_Payload payload) {
  switch ((enum IMQP_Frame_Basic)payload.method) {
  case BASIC_CONSUME:
    process_basic_consume(connection, payload.arguments.basic_consume);
    break;
  case BASIC_CONSUME_OK:
    fprintf(stderr, "[WARNING] Received a not expected basic consume OK\n");
    break;
  case BASIC_PUBLISH:
    define_pub_queue_name(payload.arguments.basic_consume.queue_name);
    break;
  default:
    THROW("Not suported");
  }
}

void define_pub_body_size(uint64_t size) { publication.size = size; }

void define_pub_body(IMQP_Byte *body) {
  memcpy(publication.body, body, publication.size);
}

void finish_pub() {
  publish();
}

int get_pub_size() { return publication.size; }

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

void process_basic_consume(Connection *connection, Basic_Consume arguments) {
  put_into_queue(connection, arguments.queue_name);
  send_basic_consume_ok(connection);

  pthread_t tid;
  pthread_create(&tid, NULL, wait_for_FIN, connection);

  connection->is_consumer = 1;
}

void process_basic_publish(Connection *connection, Basic_Publish arguments) {
  define_pub_queue_name(arguments.queue_name);
}

void define_pub_queue_name(char queue_name[MAX_QUEUE_NAME_SIZE]) {
  strcpy(publication.queue_name, queue_name);
}

void publish() {
  publish_to_queue(publication.queue_name, publication.body, publication.size);
}

void define_consumer_tag(Connection *connection) {
  const char *ctag_default = "amq.ctag-";
  const uint8_t ctag_default_size = 9;

  uint8_t random_size = CTAG_DEFAULT_RANDOM_SIZE;

  connection->consumer_tag_size = CONSUMER_TAG_SIZE;

  memcpy(connection->consumer_tag, ctag_default, ctag_default_size);

  for (uint8_t i = ctag_default_size; i < connection->consumer_tag_size; i++) {
    connection->consumer_tag[i] = (uint8_t)((rand() % ('Z' - 'A')) + 'A');
  }
}

void *wait_for_FIN(void *conn) {
  Connection* connection = (Connection*) conn;
  uint64_t socketfd = connection->socket;
  IMQP_Byte buffer[MAX_FRAME_SIZE];
  int ret = read(socketfd, buffer, MAX_FRAME_SIZE);
  if (ret > 0) {
    return wait_for_FIN(connection);
  }
  close_connection(connection);
  return NULL;
}
