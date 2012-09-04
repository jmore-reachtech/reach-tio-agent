/*
 * translate_parser.c
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* Commonly used string-handling functions */

#include "translate_parser.h"

static int gui_index = 0;
static int micro_index = 0;
//TODO: move the queue to a linked list or BST
static struct translate_queue queue;

/* we'll keep these out of the queue for now */
static struct translate_msg gui_def;
static struct translate_msg micro_def;

static void translate_msg(const char*, char*, int, struct translate_msg*,struct translate_msg);

void translate_add_mapping(const char *msg)
{
	char *origin, *key, *marker, *message, *setter;
	char tmp[MAX_LINE_SIZE];
	struct translate_msg trans;

	/* check for empty buffer */
	if(msg == NULL || *msg == '\n' || *msg == '\0') {
		return;
	}

	/* if first ch is a # or / drop it */
	if (msg[0] == '#' || msg[0] == '/') {
#ifdef DEBUG
		printf("dropping commnet: %s\n",msg);
#endif
		return;
	}

	/* copy message so we can muck with the pointer */
	strcpy(tmp,msg);

	/* default the format specifier to none */
	trans.fmt_spec = SPEC_NONE;

	/* clear the key and message*/
	memset(&trans,0,sizeof(struct translate_msg));

	/* find all the delimiters */
	origin = strtok(tmp,":");
	if(origin == NULL) {return;}

	key = strtok(NULL,",");
	if(key == NULL) {return;}

	marker = strtok(NULL,":");
	if(marker == NULL) {return;}

	message = strtok(NULL,"\n");
	if(message == NULL) {return;}

	/* check for default */
	if(key[0] == '%') {
		switch (*origin) {
		case FROM_GUI:
			strncpy(gui_def.key,key,strlen(key));
			strncpy(gui_def.msg,message,strlen(message));
			gui_def.msg[strlen(message)] = '\n';break;
		case FROM_MICRO:
			strncpy(micro_def.key,key,strlen(key));
			strncpy(micro_def.msg,message,strlen(message));
			micro_def.msg[strlen(message)] = '\n';break;
		}
		return;
	}

	/* check for = in the key and make sure we have more then just an = */
	setter = strstr(key,"=");
	if(setter != NULL && strlen(setter) > 2) {
		/* look at the setter to see if it is a format specifier */
		if (setter[1] == '%') {
			/* we have a setter so remove the specifier from the key */
			strncpy(trans.key,key,strlen(key) - strlen(setter) + 1);
			switch (setter[2]) {
			case 's':
				trans.fmt_spec = SPEC_STRING; break;
			case 'd':
				trans.fmt_spec = SPEC_INTEGER; break;
			}
			/* we need to find the format specifier */
		} else {
			/* the setter is a string so just copy the full key */
			strncpy(trans.key,key,strlen(key));
		}
	} else {
		/* copy over full key */
		strncpy(trans.key,key,strlen(key));
	}

	strncpy(trans.msg,message,strlen(message));
	/* add the \n back to the message */
	trans.msg[strlen(message)] = '\n';

	/* add message to queue*/
	switch (*origin) {
	case FROM_GUI:
		if (gui_index >= MAX_MSG_MAP_SIZE) {
#ifdef DEBUG
			printf("GUI message map is full\n");
#endif
		} else {
			queue.gui_map[gui_index] = trans; gui_index++;
		} break;
	case FROM_MICRO:
		if (micro_index >= MAX_MSG_MAP_SIZE) {
#ifdef DEBUG
			printf("Micro message map is full\n");
#endif
		} else {
			queue.micro_map[micro_index] = trans; micro_index++;
		} break;
	}
}

//TODO: check index to see if we have queued items and if not bail early
static void translate_msg(const char* msg, char* buf, int index, struct translate_msg* map, struct translate_msg def)
{
	int i;
	//const int len = strlen(msg);
	char *setter, *end_msg;
	int value;
	Boolean has_value = FALSE;
	char message[MAX_LINE_SIZE];
	char tmp[MAX_LINE_SIZE];
	
	/* if we don't have any mappings bail */
	if (index == 0) {
		strcpy(buf,msg);
		return;
	}
	
	/* check for empty message */
	if(msg == NULL || *msg == '\n' || *msg == '\0') {
		strcpy(buf,msg);;
		return;
	}
	
	/* clear the message */
	memset(message,0,MAX_LINE_SIZE);
	memset(tmp,0,MAX_LINE_SIZE);

	strcpy(tmp,msg);

	/* see if we have a setter */
	setter = strstr(tmp,"=");
	if (setter == NULL) {
		/* no setter so get up to the \n */
		setter = strtok(tmp,"\n");
		strncpy(message,setter,strlen(setter));
	} else {
		strncpy(message,msg,strlen(msg) - strlen(setter) + 1);
		/* incr pointer to get past the = */
		setter++;
		end_msg = strtok(setter,"\n");
		has_value = TRUE;
	}

	/* search our lists for the key */
	for (i = 0; i < index; i++) {
		if (strcmp(message,map[i].key) == 0) {
#ifdef DEBUG
			printf("found key %s returning msg %s\n", map[i].key, map[i].msg);
#endif
			if(has_value) {
				switch (map[i].fmt_spec) {
				case SPEC_INTEGER:
					value = atoi(end_msg);
					sprintf(buf,map[i].msg,value);break;
				case SPEC_STRING:
					sprintf(buf,map[i].msg,end_msg);break;
				case SPEC_NONE:
				default:
					/* TODO: can we even get here? */
					strcpy(buf,map[i].msg);break;
				}
			} else {
				strcpy(buf,map[i].msg);
			}
			return;
		}
	}

	/* send default message */
#ifdef DEBUG
	printf("sending default message\n");
#endif
	sprintf(buf,def.msg,tmp);
}

void traslate_gui_msg(const char* msg, char* buf)
{
	translate_msg(msg,buf,gui_index,queue.gui_map, gui_def);
}

void traslate_micro_msg(const char* msg, char* buf)
{
	translate_msg(msg,buf,micro_index,queue.micro_map, micro_def);
}

void translate_reset_mapping(void)
{
	/* reset the queue */
	gui_index = 0;
	micro_index = 0;
	memset(&queue,0,sizeof(struct translate_queue));
}
