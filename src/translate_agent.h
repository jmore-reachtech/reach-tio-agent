/*
 * translate_agent.h
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */

#ifndef TRANSLATE_AGENT_H_
#define TRANSLATE_AGENT_H_

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define BACKLOG 5
#define READ_BUF_SIZE 128
#define DEFAULT_REFRESH_DELAY 1

struct LineBuffer;

/* functions exported from translate_socket.c */
int tioQvSocketInit(unsigned short port, int *addressFamily,
    const char *socketPath);
int tioQvSocketAccept(int listenFd, int addressFamily);
void tioQvSocketWrite(int socketFd, const char *buf);

/* functions exported from translate_sio.c */
int tioSioSocketInit(unsigned short port, const char *socketName);
void tioSioSocketWrite(int sioSocketfd, char *buf);

/* functions exported from die_with_message.c */
void dieWithSystemMessage(const char *msg);

/* qml-viewer should use these same socket specifications */
#define	TIO_DEFAULT_AGENT_PORT 7885
#define TIO_AGENT_UNIX_SOCKET "/tmp/tioSocket"
#define TIO_DEFAULT_TRANSLATION_FILE_PATH "/application/bin/translate.txt";

/* these are presumeably the same as in sio_agent.h */
#define SIO_DEFAULT_AGENT_PORT 7880
#define SIO_AGENT_UNIX_SOCKET "/tmp/sioSocket"

/* global variables, shared among modules */
extern int tioVerboseFlag;

/* handy constants */
#define LOCALHOST_ADDR "127.0.0.1"

#endif /* TRANSLATE_AGENT_H_ */
