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

static int createUnixSocket(const char*);
static int createTCPServerSocket(unsigned short);

int tioQvSocketInit(unsigned short port, int *addressFamily,
    const char *socketPath)
{
    int sockFd = -1;
    if (port == 0) {
        if (tioVerboseFlag) {
            printf("using local socket\n");
        }
        sockFd = createUnixSocket(socketPath);
        *addressFamily = AF_UNIX;
    } else {
        sockFd = createTCPServerSocket(port);
        if (tioVerboseFlag) {
            printf("using tcp socket\n");
        }
        *addressFamily = AF_INET;
    }
    return sockFd;
}

int tioQvSocketAccept(int serverFd, int addressFamily)
{
    /* define a variable to hold the client's address for either family */
    union {
        struct sockaddr_un unixClientAddr;
        struct sockaddr_in inetClientAddr;
    } clientAddr;
    int clientLength = sizeof(clientAddr);

    const int clientFd = accept4(serverFd, (struct sockaddr *)&clientAddr,
        &clientLength, SOCK_NONBLOCK);
    if (clientFd >= 0) {
        switch (addressFamily) {
        case AF_UNIX:
            printf("Handling Unix client\n");
            break;

        case AF_INET:
            printf("Handling TCP client %s\n",
                inet_ntoa(clientAddr.inetClientAddr.sin_addr));
            break;

        default:
            break;
        }
    }

    return clientFd;
}

static int createTCPServerSocket(unsigned short port)
{
    struct sockaddr_in addr;
    int sfd;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        dieWithSystemMessage("socket()");

    int enable = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        dieWithSystemMessage("setsockopt() failed");
    }


    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        dieWithSystemMessage("bind() failed");
    }

    if (listen(sfd, BACKLOG) < 0) {
        dieWithSystemMessage("listen() failed");
    }

    return sfd;
}

static int createUnixSocket(const char* path)
{
    struct sockaddr_un addr;
    int sfd;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        dieWithSystemMessage("socket()");
    }

    /* Construct server socket address, bind socket to it,
       and make this a listening socket */

    if (remove(path) == -1 && errno != ENOENT) {
        dieWithSystemMessage("remove()");
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        dieWithSystemMessage("bind()");
    }

    if (listen(sfd, BACKLOG) == -1) {
        dieWithSystemMessage("listen()");
    }

    return sfd;
}

void tioQvSocketWrite(int socketFd, const char* buf)
{
    const size_t cnt = strlen(buf);
    if ((cnt > 0) && send(socketFd, buf, cnt, 0) != cnt) {
        perror("send to qml-viewer socket failed");
    }
}
