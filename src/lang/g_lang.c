// Copyright (c) Tony Givargis, 2024-2026

#include "g_parser.h"
#include "g_lang.h"

struct g_lang {
	g_parser_t parser;
};

g_lang_t
g_lang_open(const char *pathname)
{
	struct g_lang *lang;

	assert( pathname && (*pathname) );

	if (!(lang = g_malloc(sizeof (struct g_lang)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(lang, 0, sizeof (struct g_lang));
	if (!(lang->parser = g_parser_open(pathname))) {
		g_lang_close(lang);
		G_TRACE("^");
		return NULL;
	}
	return lang;
}

void
g_lang_close(g_lang_t lang)
{
	if (lang) {
		g_parser_close(lang->parser);
		memset(lang, 0, sizeof (struct g_lang));
		g_free(lang);
	}
}

struct g_lang_node *
g_lang_root(g_lang_t lang)
{
	assert( lang );

	return g_parser_root(lang->parser);
}

const char * const G_LANG_OP_STR[] = {
	"",
	"EXPR_",
	"EXPR_INT",
	"EXPR_FLOAT",
	"EXPR_STRING",
	"EXPR_IDENTIFIER",
	"EXPR_ARRAY",
	"EXPR_FUNCTION",
	"EXPR_FIELD",
	"EXPR_INC",
	"EXPR_DEC",
	"EXPR_NEG",
	"EXPR_NOT",
	"EXPR_LOGIC_NOT",
	"EXPR_MUL",
	"EXPR_DIV",
	"EXPR_MOD",
	"EXPR_ADD",
	"EXPR_SUB",
	"EXPR_SHL",
	"EXPR_SHR",
	"EXPR_LT",
	"EXPR_GT",
	"EXPR_LE",
	"EXPR_GE",
	"EXPR_EQ",
	"EXPR_NE",
	"EXPR_AND",
	"EXPR_XOR",
	"EXPR_OR",
	"EXPR_LOGIC_AND",
	"EXPR_LOGIC_OR",
	"EXPR_COND",
	"EXPR_LIST",
	"END"
};
