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


static int tioSioSocketInitUnix(const char *socketName)
{
    struct sockaddr_un addr;

    /* Create server socket */
    int sioSocketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sioSocketFd == -1) {
        dieWithSystemMessage("socket()");
    }

    /* Construct server address, and make the connection */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX; 
    strncpy(addr.sun_path, socketName, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    if (connect(sioSocketFd, (struct sockaddr *)&addr,
        sizeof(struct sockaddr)) == -1) {
        /* connection to sio_agent was not established */
        close(sioSocketFd);
        sioSocketFd = -1;
    }

    return sioSocketFd;
}

static int tioSioSocketInitTcp(unsigned short port)
{
    struct sockaddr_in addr;

    /* Create server socket */
    int sioSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sioSocketFd == -1) {
        dieWithSystemMessage("socket()");
    }

    /* Construct server address, and make the connection */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST_ADDR);
    addr.sin_port = htons(port);

    if (connect(sioSocketFd, (struct sockaddr *)&addr,
        sizeof(struct sockaddr)) == -1) {
        /* connection to sio_agent was not established */
        close(sioSocketFd);
        sioSocketFd = -1;
    }

    return sioSocketFd;
}

int tioSioSocketInit(unsigned short port, const char *socketName)
{
    int sioFd = 0;

    if (port == 0) {
        sioFd = tioSioSocketInitUnix(socketName);
    } else {
        sioFd = tioSioSocketInitTcp(port);
    }

    if (sioFd >= 0) {
        /* send ping */
        tioSioSocketWrite(sioFd, "ping\n");
    }

    return sioFd;
}

int tioSioSocketRead(int socketFd, char* buf, size_t bufSize)
{
    const size_t cnt = recv(socketFd, buf, bufSize, 0);
    if (cnt <= 0) {
        if (tioVerboseFlag) {
            printf("%s(): recv() failed, client closed\n", __FUNCTION__);
        }
        close(socketFd);
        return -1;
    } else {
        /* properly terminate the string, replacing newline if there */
        cleanString(buf, cnt);
        if (tioVerboseFlag) {
            printf("received \"%s\" from sio_agent\n", buf);
        }

        return cnt;
    }
}

void tioSioSocketWrite(int sioSocketFd, char* buf)
{
    const size_t cnt = strlen(buf);
    if (send(sioSocketFd, buf, cnt, 0) != cnt) {
        perror("send to sio_agent socket failed");
    }
}
