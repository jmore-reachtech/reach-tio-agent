TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += src/logmsg.c \
    src/rb.c \
    src/read_line.c \
    src/translate_agent.c \
    src/translate_parser.c \
    src/translate_sio.c \
    src/translate_socket.c \
    src/die_with_message.c

HEADERS += src/libtree.h \
    src/read_line.h \
    src/translate_agent.h \
    src/translate_parser.h

