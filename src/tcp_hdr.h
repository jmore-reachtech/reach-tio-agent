/*
 * tcp_hdr.h
 *
 */

#ifndef TCPECHO_H_
#define TCPECHO_H_

#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */

#include <sys/un.h>
#include <sys/socket.h>
#include "read_line.h"

#define SV_SOCK_PATH "/tmp/tioSocket"

#define BUF_SIZE 128

void dieWithUserMessage(const char *msg, const char *detail);
void dieWithSystemMessage(const char *msg);

#endif /* TCPECHO_H_ */
