// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_LANG_H_
#define _G_LANG_H_

#include "g_lexer.h"

enum {
	G_LANG_OP_,
	G_LANG_OP_EXPR_,
	G_LANG_OP_EXPR_INT,
	G_LANG_OP_EXPR_FLOAT,
	G_LANG_OP_EXPR_STRING,
	G_LANG_OP_EXPR_IDENTIFIER, // definition: left
	G_LANG_OP_EXPR_ARRAY,      // left [ right ]
	G_LANG_OP_EXPR_FUNCTION,   // left ( right )
	G_LANG_OP_EXPR_FIELD,      // left . u.s
	G_LANG_OP_EXPR_INC,        // left ++, ++ right
	G_LANG_OP_EXPR_DEC,        // left --, -- right
	G_LANG_OP_EXPR_NEG,        // - right
	G_LANG_OP_EXPR_NOT,        // ~ right
	G_LANG_OP_EXPR_LOGIC_NOT,  // ! right
	G_LANG_OP_EXPR_MUL,        // left *  right
	G_LANG_OP_EXPR_DIV,        // left /  right
	G_LANG_OP_EXPR_MOD,        // left %  right
	G_LANG_OP_EXPR_ADD,        // left +  right
	G_LANG_OP_EXPR_SUB,        // left -  right
	G_LANG_OP_EXPR_SHL,        // left << right
	G_LANG_OP_EXPR_SHR,        // left >> right
	G_LANG_OP_EXPR_LT,         // left <  right
	G_LANG_OP_EXPR_GT,         // left >  right
	G_LANG_OP_EXPR_LE,         // left <= right
	G_LANG_OP_EXPR_GE,         // left >= right
	G_LANG_OP_EXPR_EQ,         // left == right
	G_LANG_OP_EXPR_NE,         // left != right
	G_LANG_OP_EXPR_AND,        // left &  right
	G_LANG_OP_EXPR_XOR,        // left ^  right
	G_LANG_OP_EXPR_OR,         // left |  right
	G_LANG_OP_EXPR_LOGIC_AND,  // left && right
	G_LANG_OP_EXPR_LOGIC_OR,   // left || right
	G_LANG_OP_EXPR_COND,       // cond ? left : right
	G_LANG_OP_EXPR_LIST,       // expr: cond, link: right
	G_LANG_END
};

struct g_lang_node {
	int id;
	int op;
	struct g_lang_node *cond;
	struct g_lang_node *left;
	struct g_lang_node *right;
	const struct g_lexer_token *token;
};

typedef struct g_lang *g_lang_t;

extern const char * const G_LANG_OP_STR[];

g_lang_t g_lang_open(const char *pathname);

void g_lang_close(g_lang_t lang);

struct g_lang_node *g_lang_root(g_lang_t lang);

#endif
