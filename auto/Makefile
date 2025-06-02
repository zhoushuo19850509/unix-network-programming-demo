include Make.defines

PROGS = tcp_echo_server day_time_server 

all:	${PROGS}


tcp_echo_server: tcp_echo_server.o
		${CC} ${CFLAGS} -o $@ tcp_echo_server.o ${LIBS}


day_time_server: day_time_server.o
		${CC} ${CFLAGS} -o $@ day_time_server.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
