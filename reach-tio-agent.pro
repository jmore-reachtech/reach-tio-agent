TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

VERSION = 1.1.1
# add #define for the version
DEFINES += TIO_VERSION=\\\"$$VERSION\\\"

TARGET=tio-agent

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

