/*
 * die_with_message.c
 *
 */


#include <stdio.h>
#include <stdlib.h>

void dieWithSystemMessage(const char *msg)
{
	perror(msg);
	exit (1);
}
