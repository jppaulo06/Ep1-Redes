#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

void print_errors_or_warnings(uint64_t flags);
void print_warnings(uint64_t flags);
void print_errors(uint64_t flags);

#define ERRORS (((uint64_t)~0 >> 32) << 32)
#define FIRST_ERROR ((uint64_t)1 << 32)
#define LAST_ERROR ((uint64_t)1 << 63)

enum Error {
  NO_ERROR = 0,
  NOT_EXPECTED_METHOD = (uint64_t)1 << 32,
  NOT_EXPECTED_CLASS = (uint64_t)1 << 33,
  INVALID_NAME_SIZE = (uint64_t)1 << 34,
  INVALID_EXCHANGE_NAME_SIZE = (uint64_t)1 << 35,
  INVALID_CONSUMER_TAG_SIZE = (uint64_t)1 << 36,
  INVALID_FRAME = (uint64_t)1 << 37,
  INVALID_FRAME_TYPE = (uint64_t)1 << 38,
  INVALID_FRAME_CHANNEL = (uint64_t)1 << 39,
  INVALID_TUNE = (uint64_t)1 << 40,
};

#define WARNINGS (~0 ^ ERRORS)
#define FIRST_WARNING 1
#define LAST_WARNING (1 << 31)

enum Warning {
  NO_WARNING = 0,
  QUEUE_NOT_FOUND_OR_EMPTY = 1,
  HEARTBEAT_IGNORED = 1 << 1,
};

#endif /* FLAGS_H */
