# Makefile melhorado para o código da aula 19
CC=gcc
CFLAGS= -Wall
AR=ar
ARFLAGS= -rcv

################################

OBJS=fake_payloads.o imqp_functions.o super_functions.o
LIB=imqp_functions.a
MAIN=ep1_main.c

################################

.PHONY: clean

all: ep1

imqp_functions.o: imqp.h super_header.h

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} ${LIB} ${OBJS}

ep1: ${OBJS} ${LIB} ${MAIN} imqp.h
	${CC} ${CFLAGS} -o ep1 ${MAIN} ${LIB}

clean:
	@rm -rf *.o *.a ep1
