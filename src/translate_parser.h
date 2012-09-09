/*
 * translate_parser.h
 *
 *  Created on: Oct 7, 2011
 *      Author: jhorn
 */

#ifndef TRANSLATE_PARSER_H_
#define TRANSLATE_PARSER_H_

#define MAX_MSG_MAP_SIZE 200
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

struct translate_msg {
    char key[127];
    char msg[127];
    format_spec fmt_spec;
};

struct translate_queue {
    struct translate_msg gui_map[MAX_MSG_MAP_SIZE];
    struct translate_msg micro_map[MAX_MSG_MAP_SIZE];
};

void translate_add_mapping(const char*);
void translate_gui_msg(const char*, char*);
void translate_micro_msg(const char*, char*);
void translate_reset_mapping(void);

#endif /* TRANSLATE_PARSER_H_ */
