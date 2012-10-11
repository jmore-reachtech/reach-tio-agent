/* read_line.h

   Header file for read_line.c.
*/
#ifndef READ_LINE_H
#define READ_LINE_H

#include <sys/types.h>

#include "translate_agent.h"

ssize_t readLine(int fd, char *buffer, size_t n);
int readLine2(int socketFd, char *outMsg, size_t msgSize,
    struct LineBuffer *buffer, const char *end);
void safe_strncpy(char *dest, const char *src, size_t n);

struct LineBuffer
{
    char store[READ_BUF_SIZE];
    off_t pos;
};

#endif
