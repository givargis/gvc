// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_PARSER_H_
#define _G_PARSER_H_

#include "g_lang.h"
#include "g_lexer.h"

typedef struct g_parser *g_parser_t;

g_parser_t g_parser_open(const char *pathname);

void g_parser_close(g_parser_t parser);

struct g_lang_node *g_parser_root(g_parser_t parser);

#endif
