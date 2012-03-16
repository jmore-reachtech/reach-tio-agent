CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lpthread

SOURCES=die_with_message.c \
        read_line.c \
        translate_agent.c \
        translate_parser.c \
        translate_sio.c \
        translate_socket.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=tio-agent

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o: $(CC) $(CFLAGS) $(DEBUG) $< -o $@

clean:
	rm -rf *.o tio-agent


