# Makefile melhorado para o código da aula 19
CC=gcc
CFLAGS= -Wall -g3 -O0
AR=ar
ARFLAGS= -rcv

################################

CLASSES=Channel.o Connection.o Frame.o Queue.o Basic.o
OBJS=${CLASSES} super_functions.o Flags.o
LIB=imqp_functions.a
MAIN=ep1_main.c

################################

.PHONY: clean

all: ep1

Channel.o: Channel.h Frame.h super_header.h IMQP.h
Connection.o: Connection.h Frame.h super_header.h IMQP.h
Frame.o: Frame.h Connection.h Queue.h Channel.h super_header.h Basic.h IMQP.h
Queue.o: Queue.h Frame.h super_header.h IMQP.h
Basic.o: Basic.h Queue.h Frame.h super_header.h IMQP.h
super_functions.o: super_header.h IMQP.h
Flags.o: Flags.h IMQP.h

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} ${LIB} ${OBJS}

ep1: ${OBJS} ${LIB} ${MAIN} Connection.h
	${CC} ${CFLAGS} -o ep1 ${MAIN} ${LIB}

clean:
	@rm -rf *.o *.a *.log ep1
