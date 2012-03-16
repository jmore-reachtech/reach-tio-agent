/*
 * translate_sio.c
 *
 *  Created on: Oct 11, 2011
 *      Author: jhorn
 */
#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>


#include "translate_agent.h"
#include "translate_parser.h"
#include "read_line.h"

static void *translateSioReader(void *arg);

static int sioSocketFd;
pthread_t	sioSocketReader_thread;

int translateSioInit(void)
{
	struct sockaddr_in addr;

	sioSocketFd = socket(AF_INET, SOCK_STREAM, 0);      /* Create server socket */
	if (sioSocketFd == -1)
		dieWithSystemMessage("socket()");

	/* Construct server address, and make the connection */
	 memset(&addr, 0, sizeof(addr));
	 addr.sin_family = AF_INET;
	 addr.sin_addr.s_addr = inet_addr(IO_AGENT_HOST);
	 addr.sin_port = htons(SIO_AGENT_PORT);

	if (connect(sioSocketFd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
		dieWithSystemMessage("sio connect() failed");
	}

	/* send ping */
	translateSioWriter("ping\n");

	pthread_create( &sioSocketReader_thread, NULL, translateSioReader, NULL );

	return (0);
}

static void *translateSioReader(void *arg)
{
	ssize_t numRead;
	int msgRead;
	char buf[READ_BUF_SIZE];
	char msg[READ_BUF_SIZE];

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	msgRead = 0;
	for(;;) {
		while ((numRead = readLine(sioSocketFd, buf, READ_BUF_SIZE)) > 0) {
			/* zero out message buffer */
			memset(&msg,0,sizeof(&msg));
			traslate_micro_msg(buf,msg);

			/* if we have a message send */
			if(msg[0] != '\0') {
				printf("sending message to gui: %s\n",msg);
				translateSocketSendToClient(msg);
			}
			msgRead++;
		}

		if (numRead == -1)
			dieWithSystemMessage("read()");
	}

	if (close(sioSocketFd) == -1)
		dieWithSystemMessage("close()");
		
	return NULL;
}

void translateSioShutdown()
{
	pthread_cancel( sioSocketReader_thread );

    close(sioSocketFd);

}


int translateSioWriter(char* buf)
{
	int cnt = strlen(buf);

	if (send(sioSocketFd, buf, cnt, 0) != cnt ) {
		printf( "socket_send_to_client(): send() failed, %d\n", sioSocketFd );
		perror( "what's messed up?" );
	}

	return (0);
}
