/*
 * translate_agent.c
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */

#define _XOPEN_SOURCE 400

#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

#include "translate_agent.h"
#include "translate_parser.h"
#include "read_line.h"

static int keepGoing;
static const char *progName;

static void tioDumpHelp();
static void tioAgent(const char *translatePath, unsigned refreshDelay,
    unsigned short tioPort, const char *tioSocketPath, unsigned short sioPort,
    const char *sioSocketPath, const unsigned short mapSize);
static inline int max(int a, int b) { return (a > b) ? a : b; }

int main(int argc, char** argv)
{
    const char *transFilePath = TIO_DEFAULT_TRANSLATION_FILE_PATH;
    unsigned refreshDelay = 0;   /* in seconds, 0 = disabled */
    const char *logFilePath = 0;
    /* 
     * syslog isn't installed on the target so it's disabled in this program
     * by requiring an argument to -o|--log.
     */ 
    int logToSyslog = 0;
    unsigned short sioPort = 0;  /* 0 means use Unix socket */
    unsigned short tioPort = 0;
    int daemonFlag = 0;
    int verboseFlag = 0;
    unsigned short mapSize = MAX_MSG_MAP_SIZE;

    /* allocate memory for progName since basename() modifies it */
    const size_t nameLen = strlen(argv[0]) + 1;
    char arg0[nameLen];
    memcpy(arg0, argv[0], nameLen);
    progName = basename(arg0);

    while (1) {
        static struct option longOptions[] = {
            { "daemon",     no_argument,       0, 'd' },
            { "file",       required_argument, 0, 'f' },
            { "map-size",   optional_argument, 0, 'm' },
            { "refresh",    optional_argument, 0, 'r' },
            { "sio_port",   optional_argument, 0, 's' },
            { "tio_port",   optional_argument, 0, 't' },
            { "verbose",    no_argument,       0, 'v' },
            { "help",       no_argument,       0, 'h' },
            { 0,            0, 0,  0  }
        };
        int c = getopt_long(argc, argv, "df:m:r::s::t::vh?", longOptions, 0);

        if (c == -1) {
            break;  // no more options to process
        }

        switch (c) {
        case 'd':
            daemonFlag = 1;
            break;

        case 'f':
            transFilePath = optarg;
            break;

        case 'm':
            mapSize = (optarg == 0) ? MAX_MSG_MAP_SIZE : atoi(optarg);
            break;

        case 'r':
            refreshDelay = (optarg == 0) ? DEFAULT_REFRESH_DELAY : atoi(optarg);
            break;

        case 's':
            sioPort = (optarg == 0) ? SIO_DEFAULT_AGENT_PORT : atoi(optarg);
            break;

        case 't':
            tioPort = (optarg == 0) ? TIO_DEFAULT_AGENT_PORT : atoi(optarg);
            break;

        case 'v':
            verboseFlag = 1;
            break;

        case 'h':
        case '?':
        default:
            tioDumpHelp();
            exit(1);
        }
    }

    /* set up logging to syslog or file; will be STDERR not told otherwise */
    LogOpen(progName, logToSyslog, logFilePath, verboseFlag);

    /* keep STDIO going for now */
    if (daemonFlag) {
        if (daemon(0, 1) != 0) {
            dieWithSystemMessage("daemon() failed");
        }
    }

    tioAgent(transFilePath, refreshDelay, tioPort, TIO_AGENT_UNIX_SOCKET,
        sioPort, SIO_AGENT_UNIX_SOCKET, mapSize);

    exit(EXIT_SUCCESS);
}

static void tioDumpHelp()
{
	fprintf(stderr, "TIO Agent %s \n\n",TIO_VERSION);
	
    fprintf(stderr, "usage: %s [options]\n"
        "  where options are:\n"
        "    -d            | --daemon               run in background\n"
        "    -f<path>      | --file=<path>          use <file> for translations\n"
        "    -m<map size>  | --map-size=<map-size>  used for translations\n"
        "    -r<delay>     | --refresh=<delay>      autorefresh translation file\n"
        "    -s[<port>]    | --sio-port[=<port>]    use TCP socket, default = %d\n"
        "    -t[<port>]    | --tio-port[=<port>]    use TCP socket, default = %d\n"
        "    -v            | --verbose              print progress messages\n"
        "    -h            | -? | --help            print usage information\n",
        progName, SIO_DEFAULT_AGENT_PORT, TIO_DEFAULT_AGENT_PORT);
}

static void tioInterruptHandler(int sig)
{
    keepGoing = 0;
}

static void tioAgent(const char *translatePath, unsigned refreshDelay,
    unsigned short tioPort, const char *tioSocketPath, unsigned short sioPort,
    const char *sioSocketPath, const unsigned short mapSize)
{
    fd_set currFdSet;
    int connectedFd = -1;  /* not currently connected */
    time_t lastCheckTime = 0;
    time_t lastModTime = 0;
    int addressFamily = 0;

    TranslatorState *translatorState = GetTranslatorState();
    initTranslations(translatorState, mapSize);

    {
        /* install a signal handler to remove the socket file */
        struct sigaction a;
        memset(&a, 0, sizeof(a));
        a.sa_handler = tioInterruptHandler;
        if (sigaction(SIGINT, &a, 0) != 0) {
            LogMsg(LOG_ERR, "[TIO] sigaction(SIGINT) failed, errno = %d\n", errno);
            exit(1);
        }
        if (sigaction(SIGTERM, &a, 0) != 0) {
            LogMsg(LOG_ERR, "[TIO] sigaction(SIGTERM) failed, errno = %d\n", errno);
            exit(1);
        }
    }

    /* buffers for collection characters from each side */
    struct LineBuffer fromSio;
    fromSio.pos = 0;
    struct LineBuffer fromQv;
    fromQv.pos = 0;

    /* do initial load, may get reloaded in while loop, below */
    lastModTime = loadTranslations(translatorState, translatePath, 0);
    lastCheckTime = time(0);

    /* 
     * This is the select loop which waits for characters to be received on the
     * sio_agent descriptor and on either the listen socket (meaning an
     * incoming connection is queued) or on a connected socket descriptor.  If
     * not connected to the sio_agent, make select() time out to keep trying to
     * open a connection to it.
     */
    /* 100ms retry timeout for opening socket to sio_agent */
    struct  timeval timeout;
    int     sioFd       = -1;
    int     listenFd    = -1;
    int     nfds        = 0;
    keepGoing = 1;
    while (keepGoing) {
        /* try opening a connection to the sio_agent */
        if (sioFd < 0) {
            sioFd = tioSioSocketInit(sioPort, sioSocketPath);
            if (sioFd >= 0) {
                FD_ZERO(&currFdSet);
                FD_SET(sioFd, &currFdSet);
                nfds = max(sioFd,
                    (connectedFd >= 0) ? connectedFd : listenFd) + 1;

                /* open socket for qml viewer */
                listenFd = tioQvSocketInit(tioPort, &addressFamily,
                    tioSocketPath);
                if (listenFd < 0) {
                    /* open failed, can't continue */
                    LogMsg(LOG_ERR, "[TIO] could not open server socket\n");
                    return;
                }
                FD_SET(listenFd, &currFdSet);
                nfds = listenFd + 1;
            }
        }

        /* figure out timeout value address */
        struct timeval *pTimeout = 0;
        if (sioFd < 0) {
            /* Linux modifies the timeout value in select() so initialize it */
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            pTimeout = &timeout;
        }

        fd_set readFdSet = currFdSet;
        const int sel = select(nfds, &readFdSet, 0, 0, pTimeout);
        if (sel == -1) {
            if (errno != EINTR) {
                dieWithSystemMessage("select() returned -1");
            }
            /* else keepGoing was set to 0 in signal handler */
        } else if (sel > 0) {
            /* check for a new connection to accept */
            if (FD_ISSET(listenFd, &readFdSet)) {
                /* new connection is here, accept it */
                connectedFd = tioQvSocketAccept(listenFd, addressFamily);
                if (connectedFd >= 0) {
                    FD_CLR(listenFd, &currFdSet);
                    FD_SET(connectedFd, &currFdSet);
                    nfds = max(sioFd, connectedFd) + 1;
                }
            }

            /* see about auto reloading the translation file */
            if ((refreshDelay > 0) &&
                (time(0) > (lastCheckTime + refreshDelay))) {
                lastModTime = loadTranslations(translatorState, translatePath, lastModTime);
                lastCheckTime = time(0);
            }

            /* check for packet received from qml-viewer */
            if ((connectedFd >= 0) && FD_ISSET(connectedFd, &readFdSet)) {
                /* connected qml-viewer has something to say */
                char inMsg[READ_BUF_SIZE];
                const int readCount = readLine2(connectedFd, inMsg,
                    sizeof(inMsg), &fromSio, "qml-viewer");
                if (readCount < 0) {
                    /* socket closed, stop watching this file descriptor */
                    FD_CLR(connectedFd, &currFdSet);
                    FD_SET(listenFd, &currFdSet);
                    connectedFd = -1;
                    nfds = max(sioFd, listenFd) + 1;
                } else if (readCount > 0) {
                    if (sioFd >= 0) {
                        /* 
                         * this is a normal message from qml-viewer, translate
                         * it and send the result to sio_agent 
                         */ 
                        char outMsg[READ_BUF_SIZE];
                        translate_gui_msg(translatorState, inMsg, outMsg,
                            sizeof(outMsg));
                        tioSioSocketWrite(sioFd, outMsg);
                        tioSioSocketWrite(sioFd, "\r");
                    }
                }
            }

            /* check for anything from sio_agent connection */
            if ((sioFd >= 0) && FD_ISSET(sioFd, &readFdSet)) {
                /* 
                 * sio_agent socket port has something to send to the 
                 * tio_agent, if connected 
                 */
                char inMsg[READ_BUF_SIZE];
                int readCount = readLine2(sioFd, inMsg, sizeof(inMsg),
                    &fromQv, "sio-agent");
                if (readCount < 0) {
                    /* fall out of this loop to reopen connection to sio_agent */
                    FD_CLR(sioFd, &currFdSet);
                    sioFd = -1;
                    if(connectedFd >=0) {
                        close(connectedFd);
                    }
                    connectedFd = -1;
                    FD_CLR(connectedFd, &currFdSet);
                    if(listenFd >=0) {
                        close(listenFd);
                    }
                    listenFd = -1;
                    FD_CLR(listenFd, &currFdSet);
                    nfds = 0;
                } else if ((readCount > 0) && (connectedFd >= 0)) {
                    char outMsg[READ_BUF_SIZE];
                    translate_micro_msg(translatorState, inMsg, outMsg,
                        sizeof(outMsg));
                    tioQvSocketWrite(connectedFd, outMsg);
                    tioQvSocketWrite(connectedFd, "\n");
                }
            }
        } /* else timeout to retry opening sio_agent socket */
    }

    LogMsg(LOG_INFO, "[TIO] cleaning up\n");
    freeTranslations(translatorState);

    if (connectedFd >= 0) {
        close(connectedFd);
    }
    if (listenFd >= 0) {
        close(listenFd);
    }
    if (sioFd >= 0) {
        close(sioFd);
    }
    
    if (tioPort == 0) {
        /* best effort removal of socket */
        const int rv = unlink(tioSocketPath);
        if (rv == 0) {
            LogMsg(LOG_INFO, "[TIO] socket file %s unlinked\n", tioSocketPath);
        } else {
            LogMsg(LOG_INFO, "[TIO] socket file %s unlink failed\n", tioSocketPath);
        }
    }

}

