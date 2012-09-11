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

static void translate_msg(const char *, char *, int, struct translate_msg *,
    struct translate_msg);
static void safe_strncpy(char *dest, const char *src, size_t n);

void translate_add_mapping(const char *msg)
{
    char *origin, *key, *marker, *message, *setter;
    char tmp[MAX_LINE_SIZE];
    struct translate_msg trans;

    /* check for empty buffer */
    if (msg == NULL || *msg == '\n' || *msg == '\0') {
        return;
    }

    /* if first ch is a # or / drop it */
    if (msg[0] == '#' || msg[0] == '/') {
#ifdef DEBUG
        printf("dropping comment: %s\n",msg);
#endif
        return;
    }

    /* copy message so strtok() can muck with the string */
    safe_strncpy(tmp, msg, sizeof(tmp));

    /* default the format specifier to none */
    trans.fmt_spec = SPEC_NONE;

    /* clear the key and message */
    memset(&trans, 0, sizeof(struct translate_msg));

    /*
     * Translations are one per line in the form of:
     * O:K,M:G\n
     *
     * where:
     *  O = origin (G = GUI, M = micro)
     *  K = key, a string to match* or % for default translation
     *  M = marker
     *  G = message
     *
     *  * The key can contain a "setter" of the form "=%d" or "=%s" which
     *    allows for numeric or string substitutions into the message.
     */

    /* find all the delimiters */
    origin = strtok(tmp, ":");
    if (origin == NULL) {
        return;
    }

    key = strtok(NULL,",");
    if (key == NULL) {
        return;
    }

    marker = strtok(NULL,":");
    if (marker == NULL) {
        return;
    }

    message = strtok(NULL,"\n");
    if (message == NULL) {
        return;
    }

    /* check for default */
    if (key[0] == '%') {
        switch (*origin) {
        case FROM_GUI:
            safe_strncpy(gui_def.key, key, sizeof(gui_def.key));
            snprintf(gui_def.msg, sizeof(gui_def.msg), "%s\n", message);
            break;

        case FROM_MICRO:
            safe_strncpy(micro_def.key, key, sizeof(micro_def.key));
            snprintf(micro_def.msg, sizeof(micro_def.msg), "%s\n", message);
            break;

        default:
            break;
        }

        return;
    }

    /* check for = in the key and make sure we have more than just an = */
    setter = strstr(key, "=");
    if (setter != NULL && strlen(setter) > 2) {
        /* look at the setter to see if it is a format specifier */
        if (setter[1] == '%') {
            /* we have a setter so remove the specifier from the key */
            safe_strncpy(trans.key, key, setter - key + 2);
            switch (setter[2]) {
            case 's':
                trans.fmt_spec = SPEC_STRING; break;
            case 'd':
                trans.fmt_spec = SPEC_INTEGER; break;
            }
            /* we need to find the format specifier */
        } else {
            /* the setter is a string so just copy the full key */
            safe_strncpy(trans.key, key, strlen(key));
        }
    } else {
        /* copy over full key */
        safe_strncpy(trans.key, key, strlen(key));
    }

    /* copy the message over */
    snprintf(trans.msg, sizeof(trans.msg), "%s\n", message);

    /* add message to queue*/
    switch (*origin) {
    case FROM_GUI:
        if (gui_index >= MAX_MSG_MAP_SIZE) {
#ifdef DEBUG
            printf("GUI message map is full\n");
#endif
        } else {
            queue.gui_map[gui_index] = trans;
            gui_index++;
        } break;
    case FROM_MICRO:
        if (micro_index >= MAX_MSG_MAP_SIZE) {
#ifdef DEBUG
            printf("Micro message map is full\n");
#endif
        } else {
            queue.micro_map[micro_index] = trans;
            micro_index++;
        } break;
    }
}

static void translate_msg(const char* msg, char* buf, int index,
    struct translate_msg* map, struct translate_msg def)
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
        strcpy(buf, msg);
        return;
    }

    /* check for empty message */
    if (msg == NULL || *msg == '\n' || *msg == '\0') {
        strcpy(buf, msg);
        return;
    }

    /* clear the message */
    memset(message, 0, MAX_LINE_SIZE);

    /* make a copy so strtok() can change the contents */
    safe_strncpy(tmp, msg, sizeof(tmp));

    /* see if we have a setter */
    setter = strstr(tmp, "=");
    if (setter == NULL) {
        /* no setter so get up to the \n */
        setter = strtok(tmp,"\n");
        safe_strncpy(message, setter, sizeof(message));
    } else {
        strncpy(message, msg, strlen(msg) - strlen(setter) + 1);
        /* incr pointer to get past the = */
        setter++;
        end_msg = strtok(setter,"\n");
        has_value = TRUE;
    }

    /* search our lists for the key */
    for (i = 0; i < index; i++) {
        if (strcmp(message, map[i].key) == 0) {
#ifdef DEBUG
            printf("found key %s returning msg %s\n", map[i].key, map[i].msg);
#endif
            if (has_value) {
                switch (map[i].fmt_spec) {
                case SPEC_INTEGER:
                    value = atoi(end_msg);
                    sprintf(buf, map[i].msg, value);
                    break;

                case SPEC_STRING:
                    sprintf(buf, map[i].msg, end_msg);
                    break;

                case SPEC_NONE:
                default:
                    /* TODO: can we even get here? */
                    strcpy(buf, map[i].msg);
                    break;
                }
            } else {
                strcpy(buf, map[i].msg);
            }
            return;
        }
    }

    /* send default message */
#ifdef DEBUG
    printf("sending default message\n");
#endif
    sprintf(buf, def.msg, tmp);
}

void translate_gui_msg(const char* msg, char* buf)
{
    translate_msg(msg, buf, gui_index, queue.gui_map, gui_def);
}

void translate_micro_msg(const char* msg, char* buf)
{
    translate_msg(msg, buf, micro_index, queue.micro_map, micro_def);
}

void translate_reset_mapping(void)
{
    /* reset the queue */
    gui_index = 0;
    micro_index = 0;
    memset(&queue, 0, sizeof(struct translate_queue));
}

static void safe_strncpy(char *dest, const char *src, size_t n)
{
    /* This function uses strncpy() underneath but ensures that the resulting
       copy is nul terminated */
    strncpy(dest, src, n);
    dest[n - 1] = '\0';  /* if strlen(src) < n, \0 will already be in the right
                            place but this one shouldn't interfere in any way */
}

