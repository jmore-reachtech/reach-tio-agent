/*
 * translate_agent.c
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "translate_agent.h"
#include "translate_parser.h"
#include "read_line.h"

extern char *optarg;

int	verboseFlag;
int	localEchoFlag;
int	daemonFlag;
int usePort;
char socketPath[ MAX_PATH_SIZE ];
char transFilePath[ MAX_PATH_SIZE ];

static	void tioDumpHelp(void);
static void tioAgentLaunch(int);
static void loadTranslateMap(char*);

int main(int argc, char** argv)
{
	int	c;

	/* default local socket path*/
	fprintf( stderr, "tio_agent: Setting device to default /tmp/tioSocket\n" );
	fprintf( stderr, "\tUse -t devname to override\n" );
	strcpy( socketPath, "/tmp/tioSocket" );

	/* default translation file path*/
	fprintf( stderr, "tio_agent: Loading default translation file ./translate.txt\n" );
	fprintf( stderr, "\tUse -f filepath to override\n" );
	strcpy( transFilePath, "/application/bin/translate.txt" );

	/*  Options:
	 *
	 *   -d            # Run as daemon in background
	 *   -t dev        # Socket override: default /tmp/tioSocket
	 *   -f dev        # File override: default ./translate.txt
	 *   -p            # Use port and instead of local socket
	 *   -l            # Local echo on
	 *   -v            # Verbose
	 *   -h|?          # Dump Help
	 */
	opterr = 0;
	while( (c = getopt( argc, argv, "fplvdt:h?" )) != EOF ) {
		switch( c ) {
			case 'v':
				verboseFlag = 1;break;
			case 'l':
				localEchoFlag = 1;break;
			case 't':
				fprintf( stderr, "Override socket to %s\n", optarg);
				strcpy( socketPath, optarg );break;
			case 'f':
				fprintf( stderr, "Override translation file to %s\n", optarg);
				strcpy( transFilePath, optarg );break;
			case 'd':
				daemonFlag = 1; break;
			case 'p':
				usePort = 1;break;
			case '?':
			case 'h':
			default:
				tioDumpHelp();
			exit( 1 );
		}
	}

	loadTranslateMap(transFilePath);

	if(usePort) {
		tioAgentLaunch(TIO_AGENT_PORT);
	} else {
		tioAgentLaunch(0);
	}

	exit (EXIT_SUCCESS);
}

static void *tioHelpStrings[] = {

    " ",
    "usage: tio_agent [options]",
    " ",
    "-d            # Run as daemon in background",
    "-t dev        # Override /tmp/tioSocket",
    "-p port       # Use port and instead of local socket",
    "-l            # Local echo on",
    "-v            # Verbose",
    "-h|?          # Dump Help",
    " "
};

static 	void tioDumpHelp()
{
	int	i = 0;

	while( tioHelpStrings[ i ] )
		fprintf( stderr, "%s\n", tioHelpStrings[ i++ ] );

}

static void tioAgentLaunch(int port)
{
	/*  Keep STDIO going for now.
	if( daemonFlag ) {
		daemon( 0, 1 );
	}
	*/

	/*  Should never return; just wait for connections and process.
	*/
	translateSocketInit(port, socketPath );
}

static void loadTranslateMap(char* filePath)
{
	int inputFd;
	int numRead = 0;
	char buf[MAX_LINE_SIZE];

	inputFd = open(filePath, O_RDONLY);
	if (inputFd == -1) {
		printf("error opening file %s\n", filePath);
		return;
	}
	
	/* reset the queue */
	translate_reset_mapping();

	while ((numRead = readLine(inputFd, buf, MAX_LINE_SIZE)) > 0) {
		translate_add_mapping(buf);
	}

	if (numRead == -1)
		dieWithSystemMessage("read()");

	if (close(inputFd) == -1) {
		dieWithSystemMessage("error closing file");
	}
}
