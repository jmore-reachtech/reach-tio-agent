/*
 * translate_parser.h
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */

#ifndef TRANSLATE_PARSER_H_
#define TRANSLATE_PARSER_H_

#include <time.h>

/* definitions for open source lib_tree */
#include "libtree.h"

#define MAX_MSG_MAP_SIZE 400
#define MAX_LINE_SIZE 128

#define FROM_GUI 'G'
#define FROM_MICRO 'M'
#define TRANSLATE 'T'

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

typedef enum { FALSE, TRUE } Boolean;

typedef enum { SPEC_NONE, SPEC_STRING, SPEC_INTEGER } format_spec;

typedef struct TranslatorState TranslatorState;

TranslatorState *GetTranslatorState();
time_t loadTranslations(TranslatorState *state, const char* path,
    time_t lastModTime);
void translate_gui_msg(const TranslatorState *state, const char* inMsg,
    char* outMsg, size_t outMsgSize);
void translate_micro_msg(const TranslatorState *state, const char* inMsg,
    char* outMsg, size_t outMsgSize);

#endif /* TRANSLATE_PARSER_H_ */
