#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "Basic.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_CONNECTIONS 1000
#define INITIAL_MAX_CONNECTIONS_QUEUES 120

/*====================================*/
/* PRIVATE STRUCTURES */
/*====================================*/

typedef struct {
  int total_connections;
  int max_queues;
  int total_queues;
  IMQP_Queue **list;
} IMQP_Queue_List;

/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

static IMQP_Queue_List queue_list;

static pthread_mutex_t mutex_queue_declare = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_put_connection_in_queue = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_get_next_connection = PTHREAD_MUTEX_INITIALIZER;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static uint64_t process_queue_declare(Connection *connection,
                                      Queue_Declare arguments);

static void init_IMQP_Queue_List();
static IMQP_Queue *new_IMQP_Queue(char queue_name[MAX_QUEUE_NAME_SIZE]);

static uint64_t add_to_queue_list(IMQP_Queue *queue);
static void add_to_queue(IMQP_Queue *queue, Connection *connection);

static Connection *get_connection_from_queue(const char *queue_name);
static Connection *get_next_connection_from_queue(IMQP_Queue *queue);
static void print_queue_list();
static void remove_current_connection(IMQP_Queue *queue);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_frame_queue(Connection *connection, Method_Payload payload) {
  uint64_t flags = 0;
  switch ((enum IMQP_Frame_Queue)payload.method) {
  case QUEUE_DECLARE:
    flags |= process_queue_declare(connection, payload.arguments.queue_declare);
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
  }
  return flags;
}

uint64_t publish(Connection *connection) {
  Publication *pub = &(connection->publication);

  Connection *queue_connection = get_connection_from_queue(pub->queue_name);

  if (queue_connection == NULL) {
    return QUEUE_NOT_FOUND_OR_EMPTY;
  }
  send_basic_deliver(queue_connection, pub->queue_name, pub->body, pub->size);

#ifdef SNIFF_MODE
  printf("[PUBLISH]\n  Queue name: %s\n  Body: %s\n", pub->queue_name,
         pub->body);
  print_queue_list();
#endif /* SNIFF_MODE */
  return NO_ERROR;
}

uint64_t put_into_queue(Connection *connection, const char *queue_name) {
  for (int i = 0; i < queue_list.total_queues; i++) {
    IMQP_Queue *queue = queue_list.list[i];
    if (strcmp(queue->name, queue_name) == 0) {
      pthread_mutex_lock(&mutex_put_connection_in_queue);
      add_to_queue(queue, connection);
      pthread_mutex_unlock(&mutex_put_connection_in_queue);
      return NO_ERROR;
    }
  }
  return CONSUME_TO_INVALID_QUEUE;
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_queue_declare(Connection *connection,
                               Queue_Declare arguments) {
  uint64_t flags = NO_ERROR;

  pthread_mutex_lock(&mutex_queue_declare);

  if (queue_list.list == NULL) {
    init_IMQP_Queue_List();
  }
  IMQP_Queue *queue = new_IMQP_Queue(arguments.queue_name);
  flags |= add_to_queue_list(queue);
  send_queue_declare_ok(connection, queue);

#ifdef SNIFF_MODE
  print_queue_list();
#endif /* SNIFF_MODE */

  pthread_mutex_unlock(&mutex_queue_declare);

  return flags;
}

void init_IMQP_Queue_List() {
  queue_list.list =
      Malloc(sizeof(IMQP_Queue *) * INITIAL_MAX_CONNECTIONS_QUEUES);

  queue_list.max_queues = INITIAL_MAX_CONNECTIONS_QUEUES;
}

IMQP_Queue *new_IMQP_Queue(char queue_name[MAX_QUEUE_NAME_SIZE]) {
  IMQP_Queue *queue = Malloc(sizeof(*queue));

  strcpy(queue->name, queue_name);

  queue->connections = Malloc(sizeof(Connection) * INITIAL_MAX_CONNECTIONS);
  queue->max_connections = INITIAL_MAX_CONNECTIONS;
  queue->total_connections = 0;
  queue->round_robin = 0;

  return queue;
}

int queue_name_already_exists(IMQP_Queue *queue) {
  for (int i = 0; i < queue_list.total_queues; i++) {
    IMQP_Queue *saved_queue = queue_list.list[i];
    if (strcmp(saved_queue->name, queue->name) == 0) {
      return 1;
    }
  }
  return 0;
}

uint64_t add_to_queue_list(IMQP_Queue *queue) {
  if (queue_name_already_exists(queue)) {
    return QUEUE_NAME_ALREADY_EXISTS;
  }
  if (queue_list.total_queues == queue_list.max_queues) {
    queue_list.max_queues *= 2;
    queue_list.list = realloc(queue_list.list, queue_list.max_queues);
  }
  queue_list.list[queue_list.total_queues] = queue;
  queue_list.total_queues += 1;
  return NO_ERROR;
}

void add_to_queue(IMQP_Queue *queue, Connection *connection) {
  if (queue->total_connections == queue->max_connections) {
    queue->max_connections *= 2;
    queue->connections = realloc(queue->connections, queue->max_connections);
  }
  queue->connections[queue->total_connections] = connection;
  queue->total_connections += 1;

#ifdef SNIFF_MODE
  print_queue_list();
#endif /* SNIFF_MODE */
}

Connection *get_connection_from_queue(const char *queue_name) {
  for (int i = 0; i < queue_list.total_queues; i++) {
    IMQP_Queue *queue = queue_list.list[i];
    if (strcmp(queue->name, queue_name) == 0) {
      pthread_mutex_lock(&mutex_get_next_connection);
      Connection *con = get_next_connection_from_queue(queue);
      pthread_mutex_unlock(&mutex_get_next_connection);
      return con;
    }
  }
  return NULL;
}

Connection *get_next_connection_from_queue(IMQP_Queue *queue) {
  if (queue->round_robin < queue->total_connections) {
    Connection *connection = queue->connections[queue->round_robin];
    if (connection->closed) {
      remove_current_connection(queue);
      return get_next_connection_from_queue(queue);
    }
    queue->round_robin = (queue->round_robin + 1) % queue->total_connections;
    return connection;
  }
  return NULL;
}

void remove_current_connection(IMQP_Queue *queue) {
  int to_remove_index = queue->round_robin;
  Connection *to_remove_connection = queue->connections[to_remove_index];

  /* if the connection is not on the top */
  if (queue->round_robin < queue->total_connections - 1) {
    /* pull every connection* */
    for (int i = to_remove_index + 1; i < queue->total_connections; i++) {
      queue->connections[i - 1] = queue->connections[i];
    }
  } else if (queue->round_robin == queue->total_connections - 1) {
    /* reset round robin to prevent invalid memory access */
    queue->round_robin = 0;
  }

  queue->total_connections--;
  free(to_remove_connection);
}

void print_queue_list() {
  printf("[QUEUES]\n");
  for (int i = 0; i < queue_list.total_queues; i++) {
    IMQP_Queue *queue = queue_list.list[i];
    printf("  %d Name: %s\n", i, queue->name);
    for (int j = 0; j < queue->total_connections; j++) {
      if (queue->connections[j]->closed)
        continue;
      printf("    %d.%d Socket: %d\n", i, j, queue->connections[j]->socket);
    }
  }
}
