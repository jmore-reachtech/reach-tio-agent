/* read_line.h

   Header file for read_line.c.
*/
#ifndef READ_LINE_H
#define READ_LINE_H

#include <sys/types.h>

ssize_t readLine(int fd, char *buffer, size_t n);
void safe_strncpy(char *dest, const char *src, size_t n);
size_t cleanString(char *str, size_t msgLen);

#endif
