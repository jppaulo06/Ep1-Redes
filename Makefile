# Makefile melhorado para o código da aula 19
CC=gcc
CFLAGS= -Wall
AR=ar
ARFLAGS= -rcv

################################

OBJS=fake_payloads.o amqp_functions.o
DOXYFILE=aula19-Doxyfile
LIB=amqp_functions.a
MAIN=aula19-main.c

################################

.PHONY: clean

all: ep1

amqp_functions.o: amqp.h

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} ${LIB} ${OBJS}

ep1: ${OBJS} ${LIB} ${MAIN}
	${CC} ${CFLAGS} -o par_impar ${MAIN} ${LIB}

clean:
	@rm -rf *.o *.a ep1
