/* read_line.h

   Header file for read_line.c.
*/
#ifndef READ_LINE_H
#define READ_LINE_H

#include <sys/types.h>

ssize_t readLine(int fd, void *buffer, size_t n);

#endif
