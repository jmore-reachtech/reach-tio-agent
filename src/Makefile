sources = die_with_message.c \
	read_line.c \
	read_line.h \
	translate_agent.c \
	translate_agent.h \
	translate_parser.c \
	translate_parser.h \
	translate_sio.c \
	translate_socket.c 

LDFLAGS=-lpthread

all: tio-agent

tio-agent: $(sources)
	gcc $(LDFLAGS) -g -O0 -o $@ $(sources)

clean:
	-rm tio-agent

.PHONY: all clean
