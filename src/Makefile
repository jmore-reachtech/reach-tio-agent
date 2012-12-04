sources = die_with_message.c \
	read_line.c \
	translate_agent.c \
	translate_parser.c \
	translate_sio.c \
	translate_socket.c \
	rb.c \
	logmsg.c

headers = read_line.h \
	tcp_hdr.h \
	translate_agent.h \
	translate_parser.h \
	libtree.h

LDFLAGS=-pthread

CFLAGS=-Wall


ifeq ($(DEBUG_DEF),1)
	DEBUG = -g -O0 -DDEBUG
else
	DEBUG = -O2
endif


all: tio-agent

tio-agent: $(sources) $(headers)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o $@ $(sources)

clean:
	$(RM) tio-agent

.PHONY: all clean
