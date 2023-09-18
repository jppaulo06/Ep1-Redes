#include <stdio.h>
#include <string.h>

#include "Frame.h"
#include "Queue.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_CONNECTIONS 1000
#define INITIAL_MAX_CONNECTIONS_QUEUES 20

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Queue {
  QUEUE_DECLARE = 10,
  QUEUE_DECLARE_OK,
};

/*====================================*/
/* PRIVATE STRUCTURES */
/*====================================*/

typedef struct {
  uint16_t ticket;
  uint8_t name_size;
  char *name;
  int max_connections;
  int total_connections;
  int round_robin;
  Connection **connections;
} IMQP_Queue;

typedef struct {
  int total_connections;
  int max_queues;
  int total_queues;
  IMQP_Queue **list;
} IMQP_Queue_List;

/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

static IMQP_Queue_List *queue_list;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static IMQP_Queue_List *new_IMQP_Queue_List();
static IMQP_Queue *new_IMQP_Queue(IMQP_Argument *argument);
static void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue);

static void add_to_queue_list(IMQP_Queue *queue);
static void add_to_queue(IMQP_Queue *queue, Connection *connection);

static Connection *get_next_connection_from_queue(IMQP_Queue *queue);
static void print_queue_list();

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_queue(Connection *connection, IMQP_Frame *frame) {
  IMQP_Argument *arguments = frame->arguments;
  IMQP_Queue *queue = NULL;
  switch ((enum IMQP_Frame_Queue)frame->method) {
  case QUEUE_DECLARE:
    if (queue_list == NULL) {
      queue_list = new_IMQP_Queue_List();
    }
    print_queue_list();
    queue = new_IMQP_Queue(arguments);
    add_to_queue_list(queue);
    send_queue_declare_ok(connection, queue);
    break;
  case QUEUE_DECLARE_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

Connection *get_connection_from_queue(const char *queue_name) {
  for (int i = 0; i < queue_list->total_queues; i++) {
    IMQP_Queue *queue = queue_list->list[i];
    if (strcmp(queue->name, queue_name) == 0) {
      return get_next_connection_from_queue(queue);
    }
  }
  return NULL;
}

void put_into_queue(Connection *connection, const char *queue_name) {
  for (int i = 0; i < queue_list->total_queues; i++) {
    IMQP_Queue *queue = queue_list->list[i];
    if (strcmp(queue->name, queue_name) == 0) {
      add_to_queue(queue, connection);
    }
  }
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

IMQP_Queue_List *new_IMQP_Queue_List() {
  queue_list = Malloc(sizeof(*queue_list));
  queue_list->list =
      Malloc(sizeof(IMQP_Queue *) * INITIAL_MAX_CONNECTIONS_QUEUES);

  queue_list->max_queues = INITIAL_MAX_CONNECTIONS;
  queue_list->total_queues = 0;

  return queue_list;
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

void add_to_queue_list(IMQP_Queue *queue) {
  for (int i = 0; i < queue_list->total_queues; i++) {
    IMQP_Queue *saved_queue = queue_list->list[i];
    if (strcmp(saved_queue->name, queue->name) == 0) {
      return;
    }
  }
  if (queue_list->total_queues == queue_list->max_queues) {
    queue_list->max_queues *= 2;
    queue_list->list = realloc(queue_list->list, queue_list->max_queues);
  }
  queue_list->list[queue_list->total_queues] = queue;
  queue_list->total_queues += 1;
}

void add_to_queue(IMQP_Queue *queue, Connection *connection) {
  if (queue->total_connections == queue->max_connections) {
    queue->max_connections *= 2;
    queue->connections = realloc(queue->connections, queue->max_connections);
  }
  queue->connections[queue->total_connections] = connection;
  queue->total_connections += 1;
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

Connection *get_next_connection_from_queue(IMQP_Queue *queue) {
  if (queue->round_robin < queue->total_connections) {
    Connection *connection = queue->connections[queue->round_robin];
    queue->round_robin = (queue->round_robin + 1) % queue->total_connections;
    return connection;
  }
  return NULL;
}

void print_queue_list() {
  for (int i = 0; i < queue_list->total_queues; i++) {
    IMQP_Queue *queue = queue_list->list[i];
    printf("\n%d Nome: %s\n", i, queue->name);
    for (int j = 0; j < queue->total_connections; j++) {
      printf("    %d.%d Socket: %d\n", i, j, queue->connections[j]->socket);
    }
  }
}
