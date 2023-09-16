#include <linux/limits.h>
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* modified */
#include <assert.h>

#include "amqp.h"

/*====================================*/
/* FAKE REQUESTS AND RESPONSES */
/*====================================*/

extern const char* FAKE_CONNECTION_START;
extern const char* FAKE_CONNECTION_TUNE;
extern const char* FAKE_CONNECTION_OPEN_OK;
extern const char* FAKE_CHANNEL_OPEN_OK;

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

static int receive_protocol_header(Connection *connection);

static int send_connection_start(Connection *connection);
static int receive_connection_start_ok(Connection *connection);

static int send_connection_tune(Connection *connection);
static int receive_connection_tune_ok(Connection *connection);

static int receive_connection_open(Connection *connection);
static int send_connection_open_ok(Connection *connection);

static int receive_channel_open(Connection *connection);
static int send_channel_open_ok(Connection *connection);

static void update_frames(Connection *connection);

void receive_frame(Connection *connection, IMQP_Frame *frame);

static void receive_frame_header(Connection *connection, IMQP_Frame *frame);
static void receive_frame_payload(Connection *connection, IMQP_Frame *frame);
static void receive_frame_end(Connection *connection);

static void release_frame(IMQP_Frame* frame);

void process_frame_method(IMQP_Frame *frame);

/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

static const char *possible_header = "AMQP\x00\x00\x09\x01";

/*====================================*/
/* INIT CONNECTION FUNCTIONS */
/*====================================*/

Connection *new_AMQP_Connection(int socket) {
  Connection *connection = malloc(sizeof(*connection));
  connection->socket = socket;
  init_connection(connection);
  return connection;
}

void init_connection(Connection *connection) {
  receive_protocol_header(connection);

  send_connection_start(connection);
  receive_connection_start_ok(connection);

  send_connection_tune(connection);
  receive_connection_tune_ok(connection);

  receive_connection_open(connection);
  send_connection_open_ok(connection);

  receive_channel_open(connection);
  send_channel_open_ok(connection);
}

int receive_protocol_header(Connection *connection) {
  char recvbuffer[10];
  int n;
  if ((n = read(connection->socket, recvbuffer, 10)) > 0) {
    if (strcmp(recvbuffer, possible_header) != 0) {
      errno = EIO;
      return 1;
    }
    memcpy(&connection->protocol_header, recvbuffer, n);
  } else {
    errno = EIO;
    return 1;
  }
  return 0;
}

/* CONNECTION START */

int send_connection_start(Connection *connection) {
  write(connection->socket, FAKE_CONNECTION_START, strlen(FAKE_CONNECTION_START));
  return 0;
}

int receive_connection_start_ok(Connection *connection) {
  IMQP_Frame *frame = malloc(sizeof(*frame));
  receive_frame(connection, frame);
  release_frame(frame);
  return 0;
}

/* CONNECTION TUNE */

int send_connection_tune(Connection *connection) {
  write(connection->socket, FAKE_CONNECTION_TUNE, strlen(FAKE_CONNECTION_TUNE));
  return 0;
}

int receive_connection_tune_ok(Connection *connection) {
  IMQP_Frame *frame = malloc(sizeof(*frame));
  receive_frame(connection, frame);
  release_frame(frame);
  return 0;
}

/* CONNECTION OPEN */

int receive_connection_open(Connection *connection) {
  IMQP_Frame *frame = malloc(sizeof(*frame));
  receive_frame(connection, frame);
  release_frame(frame);
  return 0;
}

int send_connection_open_ok(Connection *connection) {
  write(connection->socket, FAKE_CONNECTION_OPEN_OK, strlen(FAKE_CONNECTION_OPEN_OK));
  return 0;
}

/* CHANNEL OPEN */

int receive_channel_open(Connection *connection) {
  IMQP_Frame *frame = malloc(sizeof(*frame));
  receive_frame(connection, frame);
  release_frame(frame);
  return 0;
}

int send_channel_open_ok(Connection *connection) {
  write(connection->socket, FAKE_CONNECTION_OPEN_OK, strlen(FAKE_CONNECTION_OPEN_OK));
  return 0;
}

/*====================================*/
/* GENERAL FRAME FUNCTIONS */
/*====================================*/

void receive_frame(Connection *connection, IMQP_Frame *frame) {
  receive_frame_header(connection, frame);
  receive_frame_payload(connection, frame);
  receive_frame_end(connection);
}

void receive_frame_header(Connection *connection, IMQP_Frame *frame) {
  read(connection->socket, frame, sizeof(*frame) - sizeof(frame->payload));
  frame->payload_size = frame->payload_size - sizeof(frame->class_method) - sizeof(frame->method);
}

void receive_frame_payload(Connection *connection, IMQP_Frame *frame) {
  if (frame->payload_size > 0) {
    frame->payload = malloc(frame->payload_size);
    read(connection->socket, frame->payload, frame->payload_size);
  }
}

void receive_frame_end(Connection *connection) {
  char garbage;
  read(connection->socket, &garbage, 1);
}

void release_frame(IMQP_Frame* frame){
  free(frame->payload);
  free(frame);
}

