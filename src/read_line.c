/* 
 * read_line.c
 *
 * Implementation of readLine().
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>     /* Commonly used string-handling functions */

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

            if (ch == '\n')
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

/**
 * Modifies a string presumably received on a socket and nul 
 * terminates it at either the specified length or at the first 
 * newline, which ever comes first. 
 * 
 * @param str the input string to be cleaned up and terminated
 * @param msgLen the number of valid characters in the input 
 *               string
 * 
 * @return size_t the new length of the string, including the 
 *         terminating nul character
 */
size_t cleanString(char *str, size_t msgLen)
{
    off_t i = 0;
    while ((i < msgLen) && (str[i] != '\n') && (str[i] != '\0')) {
        i++;
    }
    str[i] = '\0';
    return i;
}

