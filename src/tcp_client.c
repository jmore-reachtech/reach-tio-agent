/*
 * tpc_client.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "tcp_hdr.h"

int main(int argc, char** argv)
{
    printf("TCP Echo Client\n");

    struct sockaddr_un addr;
    int sfd;
    int laps = 0;
    char *msg = "ping\n";

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);      /* Create client socket */
    if (sfd == -1)  {
        dieWithSystemMessage("socket");
    }

    /* Construct server address, and make the connection */

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un))
        == -1) {
        dieWithSystemMessage("connect");
    }

    while (laps < 6) {
        /* Copy stdin to socket */
//      while ((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
//          if (write(sfd, buf, numRead) != numRead)
//              dieWithSystemMessage("partial/failed write");
//
//      if (numRead == -1)
//          dieWithSystemMessage("read");

        write(sfd,msg,strlen(msg));

        laps++;
        printf("resting now\n");
        sleep (10);
    }

    exit(EXIT_SUCCESS);         /* Closes our socket; server sees EOF */
}
