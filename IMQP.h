#ifndef IMQP_H
#define IMQP_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdint.h>

/*====================================*/
/* PUBLIC MACROS */
/*====================================*/

#define BASE_COMMUNICATION_CHANNEL 0
#define UNIQUE_COMMUNICATION_CHANNEL 1
#define MAX_FRAME_SIZE 3000

/*====================================*/
/* FUNDAMENTALS FRAME STRUCTURES */
/*====================================*/

typedef char IMQP_Argument;

typedef struct {
  uint8_t type;
  uint16_t channel;
  uint32_t arguments_size;
  uint16_t class_method;
  uint16_t method;
  IMQP_Argument *arguments;
} IMQP_Frame;

/*====================================*/
/* FUNDAMENTALS CONNECTION STRUCTURES */
/*====================================*/

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat;
} Config;

typedef struct {
  int socket;
  Config config;
} Connection;

#endif /* IMQP_H */
