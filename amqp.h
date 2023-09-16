#ifndef AMQP_H
#define AMQP_H

// read header of format
// AMQP0091

// frame-end sempre será o hexadecimal %xCE

/*
  Class id values from %x00.01-%xEF.FF are reserved for AMQP standard classes.
  Class id values from %xF0.00-%xFF.FF (%d61440-%d65535) may be used by
  implementations for non-standard extension classes.
 */

/*
  Integers and string lengths are always unsigned and held in network byte order
  ---> Fazer um ntoh() para interpretar os numeros corretamente.

 * AMQP strings are variable length and represented by an integer length
 followed by zero or more octets of
 * data.
 *
 * Max frame size is passed on Connection.tune
 */

#include <stdint.h>

#define AMQP_PORT 5673
#define MAX_PAYLOAD_STORAGE 300000
#define MAX_FRAME_SIZE 1 << 32

#define INITIAL_MAX_FRAMES 1000

enum AMQP_Frame_t {
  METHOD_FRAME = 1,
  HEADER_FRAME,
  BODY_FRAME,
  HEARTBEAT_FRAME
};

enum AMQP_Method {
  METHOD_connection_start = 10,
  METHOD_connection_start_ok,
  METHOD_connection_tune = 30,
  METHOD_connection_tune_ok,
  METHOD_connection_open = 30,
  METHOD_connection_open_ok,
};

typedef char IMQP_Payload;

/* Assumes the program will only receive methods */
typedef struct {
  uint8_t type;
  uint16_t channel;
  uint32_t payload_size;
  uint16_t class_method;
  uint16_t method;
  IMQP_Payload *payload;
} IMQP_Frame;

typedef struct {
  uint8_t protocol_header;
  int socket;
} Connection;

void init_connection(Connection *connection);

Connection *new_AMQP_Connection(int socket);

#endif /* AMQP_H */
