/* 
 * read_line.c
 *
 * Implementation of readLine().
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>     /* Commonly used string-handling functions */
#include <sys/socket.h>
#include <unistd.h>

#include "read_line.h"

/* 
 * Read characters from 'fd' until a newline is encountered. If a newline
 * character is not encountered in the first (n - 1) bytes, then the excess
 * characters are discarded. The returned string placed in 'buf' is
 * null-terminated and includes the newline character if it was read in the
 * first (n - 1) bytes. The function return value is the number of bytes
 * placed in buffer (which includes the newline character if encountered,
 * but excludes the terminating null byte). 
 */ 

ssize_t readLine(int fd, char *buffer, size_t n)
{
    ssize_t numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total bytes read so far */
    char ch;

    if (n <= 0 || buffer == 0) {
        errno = EINVAL;
        return -1;
    }

    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1);

        if (numRead == -1) {
            if (errno == EINTR)         /* Interrupted --> restart read() */
                continue;
            else
                return -1;              /* Some other error */

        } else if (numRead == 0) {      /* EOF */
            if (totRead == 0)           /* No bytes read; return 0 */
                return 0;
            else                        /* Some bytes read; add '\0' */
                break;

        } else {                        /* 'numRead' must be 1 if we get here */
            if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
                totRead++;
                *buffer++ = ch;
            }

            if (ch == '\n' || ch == '\r')
                break;
        }
    }

    *buffer = '\0';
    return totRead;
}

void safe_strncpy(char *dest, const char *src, size_t n)
{
    /* This function uses strncpy() underneath but ensures that the resulting
       copy is nul terminated */
    strncpy(dest, src, n);
    dest[n - 1] = '\0';  /* if strlen(src) < n, \0 will already be in the right
                            place but this one shouldn't interfere in any way */
}

int readLine2(int socketFd, char *outMsg, size_t msgSize,
    struct LineBuffer *buffer, const char *end)
{
    /* read into a temporary buffer for further processing */
    const size_t cnt = recv(socketFd, buffer->store + buffer->pos,
        sizeof(buffer->store) - buffer->pos, 0);
    if (cnt <= 0) {
        LogMsg(LOG_INFO, "recv() from %s failed, client closed\n", end);
        close(socketFd);
        buffer->pos = 0;  /* flush any remaining buffered characters */
        return -1;
    } else {
        /* look through the new part of the buffer for a newline */
        off_t i = buffer->pos;
        while (i < (buffer->pos + cnt)) {
            /* make sure outMsg is long enough to hold this message + \0 */
            if (i >= (msgSize - 1)) {
                buffer->pos = 0;
                return 0;
            }

            if (buffer->store[i] == '\n' || buffer->store[i] == '\r') {
                /* copy the accumulated characters including \n or \r to outMsg */
                bcopy(buffer->store, outMsg, i++);

                /* nul terminate the string and get rid of the \n or \r character */
                outMsg[i-1] = '\0';

                LogMsg(LOG_INFO, "received \"%s\" from %s\n", outMsg, end);

                /* squish whatever may remain to the start of the store */
                buffer->pos += cnt - i;
                bcopy(buffer->store + i, buffer->store, buffer->pos);

                return i;
            }

            i++;
        }

        if (i >= sizeof(buffer->store)) {
            /* the temporary buffer is full but no newline so flush it */
            LogMsg(LOG_ERR, "%s buffer overflow, flushing\n", end);
            buffer->pos = 0;
        }

        /* update position to start next message */
        buffer->pos += i;

        /* no newline found, no message to return */
        return 0;
    }
}
