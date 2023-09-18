# Makefile melhorado para o código da aula 19
CC=gcc
CFLAGS= -Wall
AR=ar
ARFLAGS= -rcv

################################

CLASSES=Channel.o Connection.o Frame.o Queue.o 
OBJS=${CLASSES} fake_payloads.o super_functions.o
LIB=imqp_functions.a
MAIN=ep1_main.c

################################

.PHONY: clean

all: ep1

Channel.o: Channel.h Frame.h super_header.h
Connection.o: Connection.h Frame.h super_header.h
Frame.o: Frame.h Connection.h Queue.h Channel.h super_header.h
Queue.o: Queue.h Frame.h super_header.h
super_functions.o: super_header.h

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} ${LIB} ${OBJS}

ep1: ${OBJS} ${LIB} ${MAIN} Connection.h
	${CC} ${CFLAGS} -o ep1 ${MAIN} ${LIB}

clean:
	@rm -rf *.o *.a *.log ep1
