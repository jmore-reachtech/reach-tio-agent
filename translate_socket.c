/*
 * translate_socket.c
 *
 *  Created on: Oct 10, 2011
 *      Author: jhorn
 */
#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */

#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "translate_agent.h"
#include "translate_parser.h"
#include "read_line.h"

static int createUnixSocket(char*);
static int createTCPServerSocket(unsigned short);
static int acceptTCPConnection(int);
static int readTCPConnection(int);

static int socketFd;

int translateSocketInit(int port, char* socketPath)
{
	char msg[MAX_LINE_SIZE];
	int sockFd, clientSockFd;
	int numRead;

	/* clear out msg*/
	memset(&msg[0], 0, sizeof(msg));

	if(port == 0) {
		printf("using local socket\n");
		sockFd = createUnixSocket(socketPath);
	} else if (port > 0) {
		sockFd = createTCPServerSocket(port);
		printf("using tcp socket\n");
	} else {
		dieWithSystemMessage("bad port: ");
	}

	for (;;) {
		clientSockFd = acceptTCPConnection(sockFd);

		/* Public Socket Descriptor for interaction with QML Viewer */
		socketFd = clientSockFd;

		/* init our SIO agent connection*/
		translateSioInit();

		numRead = readTCPConnection(clientSockFd);

		printf("read %d messages\n",numRead);

		translateSioShutdown();
	}

	return 0;
}

static int readTCPConnection(int clientSockFd)
{
	ssize_t numRead;
	int msgRead;
	char buf[READ_BUF_SIZE];
	char msg[READ_BUF_SIZE];

	msgRead = 0;
	while ((numRead = readLine(clientSockFd, buf, READ_BUF_SIZE)) > 0) {
		printf("got msg: %s",buf);
		/* zero out message buffer */
		memset(&msg,0,sizeof(&msg));

		/* if we get a heartbeat respond */
		if(strcmp(buf,"ping?\n") == 0 ) {
			send(clientSockFd, buf, strlen(buf), 0);
			continue;
		}

		traslate_gui_msg(buf,msg);

		/* if we have a message send */
		if(msg[0] != '\0') {
			printf("sending message to micro: %s\n",msg);
			translateSioWriter(msg);
		}
		msgRead++;
	}

	if (numRead == -1)
		dieWithSystemMessage("read()");

	if (close(clientSockFd) == -1)
		dieWithSystemMessage("close()");

	return msgRead;
}

static int acceptTCPConnection(int servSock)
{
	int clientSockFd;

	clientSockFd = accept(servSock, NULL, NULL);
	if (clientSockFd == -1) {
		dieWithSystemMessage("accept()");
	}

	return clientSockFd;
}

static int createTCPServerSocket(unsigned short port)
{
	struct	sockaddr_in	addr;
	int		sfd;
    
    sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		dieWithSystemMessage("socket()");
      
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        dieWithSystemMessage("bind() failed");

    if (listen(sfd, BACKLOG) < 0)
        dieWithSystemMessage("listen() failed");

    return sfd;
}

static int createUnixSocket(char* path)
{
	struct sockaddr_un addr;
	int sfd;

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1)
		dieWithSystemMessage("socket()");

	/* Construct server socket address, bind socket to it,
	   and make this a listening socket */

	if (remove(path) == -1 && errno != ENOENT)
		dieWithSystemMessage("remove()");

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
		dieWithSystemMessage("bind()");

	if (listen(sfd, BACKLOG) == -1)
		dieWithSystemMessage("listen()");

	return sfd;
}

void translateSocketSendToClient(char* buf)
{
	int cnt = strlen(buf);

	if (send( socketFd, buf, cnt, 0) != cnt ) {
		printf( "socket_send_to_client(): send() failed, %d\n", socketFd );
		perror( "what's messed up?" );
	}
}
