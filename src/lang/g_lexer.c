// Copyright (c) Tony Givargis, 2024-2026

#include "g_lexer.h"

#define N_MAPS 280

#define TLEN ( sizeof (struct g_lexer_token) )

#define ERROR(l, s)				\
	do {					\
		g_log("error: "			\
		      "lexer:%u:%u: %s",	\
		      (l)->lineno,		\
		      (l)->column,		\
		      (s));			\
		G_TRACE("lexer error");		\
	}					\
	while (0)

struct g_lexer {
	char *s;
	unsigned lineno;
	unsigned column;
	g_vector_t tokens;
	struct map {
		int op;
		const char *s;
	} maps[N_MAPS];
};

const char * const KEYWORDS[] = {
	"auto",       "break",     "case",     "char",     "const",
	"continue",   "default",   "do",       "double",   "else",
	"enum",       "extern",    "float",    "for",      "goto",
	"if",         "inline",    "int",      "long",     "register",
	"restrict",   "return",    "short",    "signed",   "sizeof",
	"static",     "struct",    "switch",   "typedef",  "union",
	"unsigned",   "void",      "volatile", "while",    "_Alignas",
	"_Alignof",   "_Atomic",   "_Bool",    "_Complex", "_Generic",
	"_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
	"_Pragma"
};

const char * const OPERATORS[] = {
	"+",  "-",   "*",  "/",  "%",   "++", "--",  "==",  "!=", ">",  "<",
	">=", "<=",  "&&", "||", "!",   "&",  "|",   "^",   "~",  "<<", ">>",
	"=",  "+=",  "-=", "*=", "/=",  "%=", "<<=", ">>=", "&=", "^=", "|=",
	"?",  ":",   ".",   "->", "{",    "}",  "[",  "]",   "(",  ")",  ",",
	";",  "...", "#"
};

static const char *
strdupl(const char *b, const char *e)
{
	size_t n;
	char *s;

	assert( b && (b < e) );

	n = e - b;
	if (!(s = g_malloc(n + 1))) {
		G_TRACE("^");
		return NULL;
	}
	memcpy(s, b, n);
	s[n] = '\0';
	return s;
}

static int
is_identifier(const char *b, const char *e)
{
	assert( b && (b < e) );

	if (('_' == (*b)) || isalpha((unsigned char)(*b))) {
		while (++b < e) {
			if (('_' != (*b)) && !isalnum((unsigned char)(*b))) {
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static void
insert(struct g_lexer *lexer, const char *s, int op)
{
	uint64_t h;

	assert( s && (*s) );
	assert( (G_LEXER_KEYWORD_ < op) && (G_LEXER_END > op) );

	h = g_hash(s, strlen(s));
	for (int i=0; i<N_MAPS; ++i) {
		uint64_t h_ = (h + i) % N_MAPS;
		if (!lexer->maps[h_].s) {
			lexer->maps[h_].op = op;
			lexer->maps[h_].s = s;
			return;
		}
	}
	G_TRACE("software bug (abort)");
	abort();
}

static int
lookup(struct g_lexer *lexer, const char *b, const char *e)
{
	uint64_t h;
	size_t n;

	assert( b && (b < e) );

	n = e - b;
	h = g_hash(b, n);
	for (int i=0; i<N_MAPS; ++i) {
		uint64_t h_ = (h + i) % N_MAPS;
		if (!lexer->maps[h_].s) {
			break;
		}
		if ((n == strlen(lexer->maps[h_].s) &&
		     !strncmp(b, lexer->maps[h_].s, n))) {
			return lexer->maps[h_].op;
		}
	}
	return 0;
}

static int
process(struct g_lexer *lexer, const char *b, const char *e)
{
	struct g_lexer_token *token;
	const char *s;
	int op;

	if (b < e) {
		if ((op = lookup(lexer, b, e))) {
			if (!(token = g_vector_append(lexer->tokens, TLEN))) {
				G_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
		}
		else if (is_identifier(b, e)) {
			if (!(s = strdupl(b, e)) ||
			    !(token = g_vector_append(lexer->tokens, TLEN))) {
				g_free((void *)s);
				G_TRACE("^");
				return -1;
			}
			token->op = G_LEXER_IDENTIFIER;
			token->u.s = s;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
		}
		else {
			ERROR(lexer, "invalid token");
			return -1;
		}
	}
	return 0;
}

static char *
process_comment(struct g_lexer *lexer, const char *b)
{
	const char *e;

	assert( ('/' == b[0]) && ('*' == b[1]) );

	e = b + 2;
	while (*e) {
		if ('\n' == (*e)) {
			lexer->lineno += 1;
			lexer->column  = 1;
			++e;
		}
		else if (('/' == e[0]) && ('*' == e[1])) {
			ERROR(lexer, "'/*' within block comment");
			return NULL;
		}
		else if (('*' == e[0]) && ('/' == e[1])) {
			lexer->column += 2;
			return (char *)(e + 2);
		}
		else {
			++e;
			++lexer->column;
		}
	}
	ERROR(lexer, "unterminated /* comment");
	return NULL;
}

static char *
process_string(struct g_lexer *lexer, const char *b)
{
	struct g_lexer_token *token;
	const char *s, *e;
	char quote;

	assert( ('\"' == b[0]) || ('\'' == b[0]) );

	e = b;
	quote = (*b);
	while ((*e) && ('\n' != (*e))) {
		++e;
		++lexer->column;
		if (quote == (*e)) {
			++e;
			++lexer->column;
			if (!(s = strdupl(b, e)) ||
			    !(token = g_vector_append(lexer->tokens, TLEN))) {
				g_free((void *)s);
				G_TRACE("^");
				return NULL;
			}
			token->op = G_LEXER_STRING;
			token->u.s = s;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
			return (char *)e;
		}
		if ('\\' == (*e)) {
			++e;
			++lexer->column;
			if (!(*e)) {
				ERROR(lexer, "missing escape character");
				return NULL;
			}
		}
	}
	ERROR(lexer, "missing terminating character");
	return NULL;
}

static char *
process_numeric(struct g_lexer *lexer, const char *b)
{
	struct g_lexer_token *token;
	const char *s, *e;
	g_bigint_t i;
	double f;

	// hexadecimal integer

	e = b;
	if (('0' == e[0]) && ('X' == toupper((unsigned char)e[1]))) {
		e += 2;
		lexer->column += 2;
		while (isxdigit((unsigned char)(*e))) {
			++e;
			++lexer->column;
		}
		if (2 >= (e - b)) {
			ERROR(lexer, "invalid integer constant");
			return NULL;
		}
		if (!(s = strdupl(b, e)) || !(i = g_bigint_string(s))) {
			g_free((void *)s);
			G_TRACE("^");
			return NULL;
		}
		g_free((void *)s);
		if (!(token = g_vector_append(lexer->tokens, TLEN))) {
			g_bigint_free(i);
			G_TRACE("^");
			return NULL;
		}
		token->op = G_LEXER_INT;
		token->u.i = i;
		token->lineno = lexer->lineno;
		token->column = lexer->column - (unsigned)(e - b);
		return (char *)e;
	}

	// real

	errno = 0;
	f = strtod(b, (char **)&e);
	if ((EINVAL == errno) || (ERANGE == errno)) {
		ERROR(lexer, "invalid real constant");
		return NULL;
	}
	for (s=b; s<e; ++s) {
		if (!isdigit((unsigned char)(*s))) {
			break;
		}
	}
	if (s < e) {
		lexer->column += (unsigned)(e - b);
		if (!(token = g_vector_append(lexer->tokens, TLEN))) {
			G_TRACE("^");
			return NULL;
		}
		token->op = G_LEXER_FLOAT;
		token->u.f = f;
		token->lineno = lexer->lineno;
		token->column = lexer->column - (unsigned)(e - b);
		return (char *)e;
	}

	// decimal integer

	e = b;
	while (isdigit((unsigned char)(*e))) {
		++e;
		++lexer->column;
	}
	if (!(s = strdupl(b, e)) || !(i = g_bigint_string(s))) {
		g_free((void *)s);
		G_TRACE("^");
		return NULL;
	}
	g_free((void *)s);
	if (!(token = g_vector_append(lexer->tokens, TLEN))) {
		g_bigint_free(i);
		G_TRACE("^");
		return NULL;
	}
	token->op = G_LEXER_INT;
	token->u.i = i;
	token->lineno = lexer->lineno;
	token->column = lexer->column - (unsigned)(e - b);
	return (char *)e;
}

static int
tokenize(struct g_lexer *lexer)
{
	struct g_lexer_token *token;
	char *e, *b;
	int op;

	e = lexer->s;
	b = lexer->s;
	lexer->lineno = 1;
	lexer->column = 1;
	while (*e) {
		if (isspace((unsigned char)(*e))) {
			if (process(lexer, b, e)) {
				G_TRACE("^");
				return -1;
			}
			if ('\n' == (*e)) {
				lexer->lineno += 1;
				lexer->column  = 1;
			}
			else {
				lexer->column += 1;
			}
			b = ++e;
		}
		else if (('/' == e[0]) && ('/' == e[1])) {
			if (process(lexer, b, e)) {
				G_TRACE("^");
				return -1;
			}
			while ((*e) && ('\n' != (*e))) {
				++e;
				++lexer->column;
			}
			b = e;
		}
		else if (('/' == e[0]) && ('*' == e[1])) {
			if (process(lexer, b, e) ||
			    !(e = process_comment(lexer, e))) {
				G_TRACE("^");
				return -1;
			}
			b = e;
		}
		else if (('"' == (*e)) || ('\'' == (*e))) {
			if (process(lexer, b, e) ||
			    !(e = process_string(lexer, e))) {
				G_TRACE("^");
				return -1;
			}
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 3)) &&
			 (G_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = g_vector_append(lexer->tokens, TLEN))) {
				G_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 3;
			e += 3;
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 2)) &&
			 (G_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = g_vector_append(lexer->tokens, TLEN))) {
				G_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 2;
			e += 2;
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 1)) &&
			 (G_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = g_vector_append(lexer->tokens, TLEN))) {
				G_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 1;
			e += 1;
			b = e;
		}
		else if ((b == e) &&
			 (isdigit((unsigned char)(*e)) ||
			  (('.' == e[0]) && isdigit((unsigned char)e[1])))) {
			if (!(e = process_numeric(lexer, e))) {
				G_TRACE("^");
				return -1;
			}
			b = e;
		}
		else {
			++e;
			++lexer->column;
		}
	}
	if (process(lexer, b, e)) {
		G_TRACE("^");
		return -1;
	}
	if (!(token = g_vector_append(lexer->tokens, TLEN))) {
		G_TRACE("^");
		return -1;
	}
	token->op = G_LEXER_END;
	token->lineno = lexer->lineno;
	token->column = lexer->column;
	return 0;
}

g_lexer_t
g_lexer_open(const char *pathname)
{
	struct g_lexer *lexer;

	assert( pathname && (*pathname) );

	if (!(lexer = g_malloc(sizeof (struct g_lexer)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(lexer, 0, sizeof (struct g_lexer));
	for (size_t i=0; i<G_ARRAY_SIZE(KEYWORDS); ++i) {
		insert(lexer, KEYWORDS[i], G_LEXER_KEYWORD_ + (int)(i + 1));
	}
	for (size_t i=0; i<G_ARRAY_SIZE(OPERATORS); ++i) {
		insert(lexer, OPERATORS[i], G_LEXER_OPERATOR_ + (int)(i + 1));
	}
	if (!(lexer->tokens = g_vector_open())) {
		g_lexer_close(lexer);
		G_TRACE("^");
		return NULL;
	}
	if (!(lexer->s = g_file_string_read(pathname))) {
		ERROR(lexer, "unable to read file");
		g_lexer_close(lexer);
		return NULL;
	}
	if (tokenize(lexer)) {
		g_lexer_close(lexer);
		G_TRACE("^");
		return NULL;
	}
	return lexer;
}

void
g_lexer_close(g_lexer_t lexer)
{
	struct g_lexer_token *token;
	uint64_t i;

	if (lexer) {
		if (lexer->tokens) {
			for (i=0; i<g_vector_items(lexer->tokens); ++i) {
				token = g_vector_lookup(lexer->tokens, i);
				if (G_LEXER_INT == token->op) {
					g_bigint_free(token->u.i);
				}
				else if ((G_LEXER_IDENTIFIER == token->op) ||
					 (G_LEXER_STRING == token->op)) {
					g_free((void *)token->u.s);
				}
			}
			g_vector_close(lexer->tokens);
		}
		g_free(lexer->s);
		memset(lexer, 0, sizeof (struct g_lexer));
		g_free(lexer);
	}
}

const struct g_lexer_token *
g_lexer_lookup(g_lexer_t lexer, uint64_t i)
{
	assert( lexer );

	return g_vector_lookup(lexer->tokens, i);
}

uint64_t
g_lexer_items(g_lexer_t lexer)
{
	assert( lexer );

	return g_vector_items(lexer->tokens);
}
