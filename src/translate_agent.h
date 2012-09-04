/*
 * translate_agent.h
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */

#ifndef TRANSLATE_AGENT_H_
#define TRANSLATE_AGENT_H_


char transFilePath[ 200 ];
char socketPath[ 200 ];

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define	SIO_AGENT_PORT 7880
#define	TIO_AGENT_PORT 7885
#define MAX_PATH_SIZE 200
#define IO_AGENT_HOST "127.0.0.1"

#define BACKLOG 5

#define READ_BUF_SIZE 128

int translateSocketInit(int, char*);
int translateSioInit(void);
int translateSioWriter(char*);
void translateSioShutdown(void);

void translateSocketSendToClient(char*);

void dieWithUserMessage(const char *msg, const char *detail);
void dieWithSystemMessage(const char *msg);

#endif /* TRANSLATE_AGENT_H_ */
