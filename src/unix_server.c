/*
 * domain_server.c
 *
 */
#include "tcp_hdr.h"
#include "read_line.h"

#define BACKLOG 5

void parse(char*);

int main(int argc, char** argv)
{
    struct sockaddr_un addr;
    int sfd, cfd;
    ssize_t numRead;
    char buf[BUF_SIZE];

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        dieWithSystemMessage("socket()");
    }

    /* Construct server socket address, bind socket to it,
       and make this a listening socket */

    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT) {
        dieWithSystemMessage("remove()");
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        dieWithSystemMessage("bind()");
    }

    if (listen(sfd, BACKLOG) == -1) {
        dieWithSystemMessage("listen()");
    }

    for (;;) {          /* Handle client connections iteratively */
        /* Accept a connection. The connection is returned on a new
           socket, 'cfd'; the listening socket ('sfd') remains open
           and can be used to accept further connections. */

        cfd = accept(sfd, NULL, NULL);
        if (cfd == -1) {
            dieWithSystemMessage("accept()");
        }

        /* Transfer data from connected socket to stdout until EOF

        while ((numRead = read(cfd, buf, BUF_SIZE)) > 0)
            if (write(STDOUT_FILENO, buf, numRead) != numRead)
                dieWithSystemMessage("partial/failed write");
        */

        while ((numRead = readLine(cfd, buf, BUF_SIZE)) > 0) {
            parse(buf);

            if (write(STDOUT_FILENO, buf, numRead) != numRead) {
                dieWithSystemMessage("partial/failed write");
            }
        }

        if (numRead == -1) {
            dieWithSystemMessage("read()");
        }

        if (close(cfd) == -1) {
            dieWithSystemMessage("close()");
        }
    }
}


void parse(char* msg)
{
    char* pch;
    pch = strtok(msg,",");
    while (pch != NULL) {
        printf("%s\n",pch);
        pch = strtok(NULL, ",");
    }
    printf("\n");
}
