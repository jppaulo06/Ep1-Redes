#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef MAX_ERROR_STRING
#define MAX_ERROR_STRING 500
#endif

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "super_header.h"
#include <sys/socket.h>

/*====================================*/
/* FUNCTIONS */
/*====================================*/

void Throw(const char *CUSTOM_ERROR_STRING, const char *file,
           const char *function, int line) {
  char *FULL_ERROR_STRING = malloc(MAX_ERROR_STRING);
  sprintf(FULL_ERROR_STRING,
          "\n========== ERROR! =========="
          "\nFile: %s\nFunction: %s\nLine: %d\nCustom description: %s\n",
          file, function, line, CUSTOM_ERROR_STRING);
  perror(FULL_ERROR_STRING);
  exit(1);
}

ssize_t Recv_func(int fildes, char *buf, size_t nbyte, int flags, const char *file,
                  const char *function, int line) {
  ssize_t n;
  if ((n = recv(fildes, (void *)buf, nbyte, flags)) >= 0) {
    return n;
  }
  Throw("Could not read from socket", file, function, line);
  return 1;
}

ssize_t Write_func(int fildes, const char *buf, size_t nbyte, const char *file,
                   const char *function, int line) {
  ssize_t n;
  if ((n = write(fildes, (void *)buf, nbyte)) > 0) {
    return n;
  }
  Throw("Could not write on socket", file, function, line);
  return 1;
}

void *Malloc_func(size_t size, const char *file, const char *function,
                  int line) {
  void *pointer;
  if ((pointer = malloc(size)) != NULL) {
    return pointer;
  }
  Throw("WTF bro go buy some memory", file, function, line);
  return NULL;
}
