/*
 * translate_parser.c
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* Commonly used string-handling functions */
#include <sys/stat.h>
#include <unistd.h>

#include "translate_parser.h"
#include "read_line.h"

/* forward function declarations */
void translate_add_mapping(TranslatorState *state, const char*,
    unsigned lineNumber);
void translate_reset_mapping(TranslatorState *state);
int compareTranslations(const struct rbtree_node *first,
    const struct rbtree_node *second);
void initTranslations(TranslatorState *state, const unsigned short mapSize);
void freeTranslations(TranslatorState *state);

int maxMappingSize = -1;

/**
 * This structure is a single record of information which is placed into a
 * red/black tree.
 */
struct translate_msg {
    char key[MAX_LINE_SIZE];
    char msg[MAX_LINE_SIZE];
    format_spec fmt_spec;
    unsigned lineNumber;
    struct rbtree_node node;
};

/**
 * This structure represents the state of the translation maps used by the 
 * agent.  It contains the free store of translations, maps for both directions 
 * of message traversal and default messages for each. 
 */
struct TranslatorState
{
    /* the number of translations allocated from translations array */
    unsigned translationCount;

    /* the default message to use for messages from the GUI */
    char guiDefault[MAX_LINE_SIZE];

    /* the default message to use for messages from the micro */
    char microDefault[MAX_LINE_SIZE];

    /* the pool of translation structures that will be put into the maps */
    struct translate_msg* translations;

    /* the map of translation messages for messages received from the GUI */
    struct rbtree guiTranslationMap;

    /* the map of translation messages for messages received from the micro */
    struct rbtree microTranslationMap;
};

/**
 * The program has one set of translations.  One such structure is statically 
 * allocated in this function and its address is returned by it in order for 
 * the program to gain access to it. 
 * 
 * @return TranslatorState* 
 */
TranslatorState *GetTranslatorState()
{
    static TranslatorState state;        
    return &state;
}

/**
 * This function will possibly load the translation maps from a file.  Whether 
 * this occurrs or not depends on the file's modification time and the time 
 * argument passed in. 
 *  
 * Typical use is to do the initial load by passing in 0 as the lastModTime 
 * then using the return value the next time the function is called.  This way 
 * the translations will be reloaded only if the file has been modified. 
 * 
 * @param state the program's set of translations, etc.
 * @param filePath the file system path name to the file containing the 
 *                 translations to be loaded
 * @param lastModTime the time of the previous check for reloading the 
 *                    translations; this can be 0 to force the load to take
 *                    place
 * 
 * @return time_t the modification time of the file specified by filePath
 */
time_t loadTranslations(TranslatorState *state, const char* filePath,
    time_t lastModTime)
{
	int inputFd;
	int numRead = 0;
	char buf[MAX_LINE_SIZE];
    struct stat filestat;

    int doReload = 0;
    if (stat(filePath, &filestat) != 0) {
        perror("stat() failed");
        memset(&filestat, 0, sizeof(filestat));
        doReload = 1;
    } else {
        doReload = filestat.st_mtime > lastModTime;
    }

    if (doReload) {
        inputFd = open(filePath, O_RDONLY);
        if (inputFd == -1) {
            LogMsg(LOG_ERR, "[TIO] error opening file %s\n", filePath);
            return 0;
        }

        /* remove all current translations */
        translate_reset_mapping(state);

        unsigned lineNumber = 1;
        while ((numRead = readLine(inputFd, buf, MAX_LINE_SIZE)) > 0) {
            //Check for windows cr
            if (buf[strlen(buf) -1] == '\r')
                buf[strlen(buf) - 1] = 0;
            translate_add_mapping(state, buf, lineNumber++);
        }

        if (numRead == -1)
            dieWithSystemMessage("read()");

        if (close(inputFd) == -1) {
            dieWithSystemMessage("error closing file");
        }

        LogMsg(LOG_INFO, "[TIO] loaded translation file \"%s\"\n", filePath);
    }

    return filestat.st_mtime;
}

/**
 * Adds a single translation to the correct map in the state object.
 * 
 * @param state the translations state for the program
 * @param msg a single line from the translation file
 * @param lineNumber the line number of the line in the translations file
 */
void translate_add_mapping(TranslatorState *state, const char *msg,
    unsigned lineNumber)
{
    char *origin, *key, *marker, *message, *setter;
    char tmp[MAX_LINE_SIZE];

    /* check for empty buffer */
    if (msg == NULL || *msg == '\n' || *msg == '\r' || *msg == '\0') {
        return;
    }

    /* if first ch is a # or / drop it */
    if (msg[0] == '#' || msg[0] == '/') {
        LogMsg(LOG_INFO, "[TIO] dropping comment on line %d: %s\n", lineNumber, msg);
        return;
    }

    /* copy message so strtok() can muck with the string */
    safe_strncpy(tmp, msg, sizeof(tmp));

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
            snprintf(state->microDefault, MAX_LINE_SIZE, "%s\n", message);
            break;

        case FROM_MICRO:
            snprintf(state->microDefault, MAX_LINE_SIZE, "%s\n", message);
            break;

        default:
            break;
        }

        return;
    }

    /* allocate a translation from array of them if any left */
    if (state->translationCount >= maxMappingSize) {
        static int errorPrinted = 0;
        if (!errorPrinted) {
            LogMsg(LOG_ERR,
                "[TIO] too many translation rules, maximum of %d allowed\n",
                maxMappingSize);
            errorPrinted = 1;
        }
    } else {
        struct translate_msg *translation =
            &state->translations[state->translationCount++];

        /* clear the key and message */
        memset(translation, 0, sizeof(struct translate_msg));

        /* set line number */
        translation->lineNumber = lineNumber;

        /* check for = in the key and make sure we have more than just an = */
        setter = strstr(key, "=");
        if (setter != NULL && strlen(setter) > 2) {
            /* look at the setter to see if it is a format specifier */
            if (setter[1] == '%') {
                /* we have a setter so remove the specifier from the key */
                safe_strncpy(translation->key, key, setter - key + 2);
                switch (setter[2]) {
                case 's':
                    translation->fmt_spec = SPEC_STRING; break;
                case 'd':
                    translation->fmt_spec = SPEC_INTEGER; break;
                }
                /* we need to find the format specifier */
            } else {
                /* the setter is a string so just copy the full key */
                safe_strncpy(translation->key, key, strlen(key) + 1);
            }
        } else {
            /* copy over full key */
            safe_strncpy(translation->key, key, strlen(key) + 1);
        }

        /* copy the message over */
        snprintf(translation->msg, sizeof(translation->msg), "%s", message);

        /* add message to map */
        const char *mapName = 0;
        struct rbtree *map = 0;
        switch (*origin) {
        case FROM_GUI:
            mapName = "GUI";
            map = &state->guiTranslationMap;
            break;

        case FROM_MICRO:
            mapName = "micro";
            map = &state->microTranslationMap;
            break;
        }
        const struct rbtree_node *ret = rbtree_insert(&translation->node, map);
        if (ret != 0) {
            const struct translate_msg *originalNode = rbtree_container_of(ret,
                struct translate_msg, node);
            LogMsg(LOG_ERR, "[TIO] translation for key \"%s\" on line %d in %s map "
                "already defined on line %d.\n", key, lineNumber, mapName,
                originalNode->lineNumber);
        }
    }
}

/**
 * Provides a translated message out from an input line. If the key part of the 
 * input message matches a key in the specified tree, the message from the map 
 * is substituted.  If no match is found, then the default message (if defined) 
 * is returned.  If the default message is zero length, the input message is 
 * returned, unchanged, in the output message. 
 * 
 * @param inMsg the message to be translated
 * @param outMsg the translated message
 * @param outMsgSize the number of characters available at outMsg
 * @param map the map to search for the message key
 * @param defaultMsg the message to use as default if the map doesn't have a 
 *                   match
 */
static void translate_msg(const char* inMsg, char* outMsg, size_t outMsgSize,
    const struct rbtree *map, const char *defaultMsg)
{
    char *setter, *end_msg = 0;
    int value;
    Boolean has_value = FALSE;
    char message[MAX_LINE_SIZE];
    char tmp[MAX_LINE_SIZE];

    /* check for empty message */
    if (inMsg == 0 || *inMsg == '\n' || *inMsg == '\r' || *inMsg == '\0') {
        strncpy(outMsg, "\n", outMsgSize);
        return;
    }

    /* if we don't have any mappings bail */
    if (rbtree_first(map) == 0) {
        safe_strncpy(outMsg, inMsg, outMsgSize);
        return;
    }

    /* clear the message */
    memset(message, 0, MAX_LINE_SIZE);

    /* make a copy so strtok() can change the contents */
    safe_strncpy(tmp, inMsg, sizeof(tmp));

    /* see if we have a setter */
    setter = strstr(tmp, "=");
    if (setter == NULL) {
        /* no setter so get up to the \n */
        setter = strtok(tmp,"\n");
        safe_strncpy(message, setter, sizeof(message));
    } else {
        strncpy(message, inMsg, strlen(inMsg) - strlen(setter) + 1);
        /* incr pointer to get past the = */
        setter++;
        end_msg = strtok(setter,"\n");
        has_value = TRUE;
    }

    /* look for the key in the map */
    struct translate_msg searchKey;
    strncpy(searchKey.key, message, sizeof(searchKey.key));
    const struct rbtree_node *node = rbtree_lookup(&searchKey.node, map);
    if (node == 0) {
        /* not found; use the default */
        if (strlen(defaultMsg) > 0) {
            LogMsg(LOG_INFO, "[TIO] sending default message\n");
            sprintf(outMsg, defaultMsg, tmp);
        } else {
            LogMsg(LOG_INFO, "[TIO] sending untranslated message\n");
            safe_strncpy(outMsg, inMsg, outMsgSize);
        }
    } else {
        /* translation found in map, format outMsg accordingly */
        const struct translate_msg *translation =
            rbtree_container_of(node, struct translate_msg, node);

        LogMsg(LOG_INFO, "[TIO] found key => \"%s\"; returning => \"%s\"\n", translation->key,
            translation->msg);

        if (has_value) {
            switch (translation->fmt_spec) {
            case SPEC_INTEGER:
                value = atoi(end_msg);
                snprintf(outMsg, outMsgSize, translation->msg, value);
                break;

            case SPEC_STRING:
                snprintf(outMsg, outMsgSize, translation->msg, end_msg);
                break;

            case SPEC_NONE:
            default:
                /* TODO: can we even get here? */
                safe_strncpy(outMsg, translation->msg, outMsgSize);
                break;
            }
        } else {
            safe_strncpy(outMsg, translation->msg, outMsgSize);
        }
        return;
    }
}

/**
 * Translate a message from the GUI to a message to be sent to the 
 * microcontroller. 
 * 
 * @param state the program's set of translations
 * @param inMsg the message from the GUI to be translated
 * @param outMsg a buffer into which a translated message is to be written
 * @param outMsgSize the maximum length of the output message
 */
void translate_gui_msg(const TranslatorState *state, const char* inMsg,
    char* outMsg, size_t outMsgSize)
{
    translate_msg(inMsg, outMsg, outMsgSize, &state->guiTranslationMap,
        state->guiDefault);
}

/**
 * Translate a message from the microcontroller to a message to be sent to the 
 * GUI. 
 * 
 * @param state the program's set of translations
 * @param inMsg the message from the microcontroller to be translated
 * @param outMsg a buffer into which a translated message is to be written
 * @param outMsgSize the maximum length of the output message
 */
void translate_micro_msg(const TranslatorState *state, const char* inMsg,
    char* outMsg, size_t outMsgSize)
{
    translate_msg(inMsg, outMsg, outMsgSize, &state->microTranslationMap,
        state->microDefault);
}

/**
 * Removes all translations.
 * 
 * @param state the program's set of translations which is to be cleared to 
 *              contain none
 */
void translate_reset_mapping(TranslatorState *state)
{
    state->translationCount = 0;
    state->guiDefault[0] = '\0';
    state->microDefault[0] = '\0';
    rbtree_init(&state->guiTranslationMap, compareTranslations, 0);
    rbtree_init(&state->microTranslationMap, compareTranslations, 0);
}

/**
 * This function is called by the red/black tree library functions during node 
 * insertion and lookups. 
 * 
 * @param first one of the two nodes to be compared
 * @param second the other node to which the first is to be compared
 * 
 * @return int <0 if first < second, >0 if first > second and 0 if first == 
 *         second
 */
int compareTranslations(const struct rbtree_node *first,
    const struct rbtree_node *second)
{
    const char *str1 = rbtree_container_of(first, struct translate_msg, node)->key;
    const char *str2 = rbtree_container_of(second, struct translate_msg, node)->key;
    return strcmp(str1, str2);
}

/**
 * This function allocates memory for a variable number of translations.
 *
 * @param state the program's set of translations
 * @param mapSize number to set total mappings in the translate file
 *
 */
void initTranslations(TranslatorState *state, const unsigned short mapSize)
{
    state->translations = malloc(mapSize * sizeof(struct translate_msg));
    maxMappingSize = mapSize;
    LogMsg(LOG_INFO, "[TIO] translations size set to %d\n", maxMappingSize);
}

/**
 * This function frees translations from memory.
 *
 * @param state the program's set of translations
 *
 */
void freeTranslations(TranslatorState *state)
{
    free(state->translations);
    LogMsg(LOG_INFO, "[TIO] translations free()\n");
}
