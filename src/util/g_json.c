// Copyright (c) Tony Givargis, 2024-2026

#include "g_json.h"

#define ERROR(j, s)					\
	do {						\
		(j)->curr = "";				\
		(j)->token.op = OP_EOF;			\
		if (!(j)->stop) {			\
			g_log("error: "			\
			      "json:%u:%u: %s",		\
			      (j)->lineno,		\
			      (j)->column,		\
			      (s));			\
			G_TRACE("syntax error");	\
		}					\
		(j)->stop = 1;				\
	}						\
	while (0)

#define MKN(j, n, op_)				\
	do {					\
		if (!((n) = allocate((j)))) {   \
			ERROR((j), "^");	\
			return NULL;		\
		}				\
		(n)->op = (op_);		\
	}					\
	while (0)

struct g_json {
	int stop;
	char *curr;
	char *content;
	unsigned lineno;
	unsigned column;
	struct g_json_node *root;
	struct {
		enum {
			OP_EOF,
			//---
			OP_NULL,
			OP_BOOL,
			OP_COMMA,
			OP_COLON,
			OP_STRING,
			OP_NUMBER,
			OP_OPEN_BRACE,
			OP_CLOSE_BRACE,
			OP_OPEN_BRACKET,
			OP_CLOSE_BRACKET
		} op;
		union {
			int bool_; // OP_BOOL
			double number; // OP_NUMBER
			const char *string; // OP_STRING
		} u;
	} token;
	//---
	struct chunk {
		struct chunk *link;
		struct g_json_node nodes[256];
	} *chunk;
	int size;
};

static struct g_json_node *
allocate(struct g_json *json)
{
	struct g_json_node *node;
	struct chunk *chunk;
	int size;

	size = (int)G_ARRAY_SIZE(chunk->nodes);
	if (!json->chunk || (size <= json->size)) {
		if (!(chunk = g_malloc(sizeof (struct chunk)))) {
			G_TRACE("^");
			return NULL;
		}
		chunk->link = json->chunk;
		json->chunk = chunk;
		json->size = 0;
	}
	node = &json->chunk->nodes[++json->size];
	memset(node, 0, sizeof (struct g_json_node));
	return node;
}

static int
ishex(char c)
{
	c = tolower(c);
	if (isdigit((unsigned char)c)) {
		return 1;
	}
	if (('a' <= c) && ('f' >= c)) {
		return 1;
	}
	return 0;
}

static char *
eat_string(char *s)
{
	int i;

	while (*++s) {
		if ('\"' == (*s)) {
			return s;
		}
		else if (0x20 > (*s)) {
			return NULL;
		}
		else if ('\\' == (*s)) {
			++s;
			if (('"' == (*s)) ||
			    ('/' == (*s)) ||
			    ('b' == (*s)) ||
			    ('f' == (*s)) ||
			    ('n' == (*s)) ||
			    ('r' == (*s)) ||
			    ('t' == (*s)) ||
			    ('\\' == (*s))) {
				continue;
			}
			else if ('u' == (*s)) {
				for (i=0; i<4; ++i) {
					if (!ishex(*++s)) {
						return NULL;
					}
				}
				continue;
			}
			else {
				return NULL;
			}
		}
	}
	return NULL;
}

static void
forward(struct g_json *json)
{
	char *s, *e;
	double r;

	s = json->curr;
	while (*s) {
		if (isspace((unsigned char)(*s))) {
			if ('\n' == (*s)) {
				json->column  = 0;
				json->lineno += 1;
			}
			++json->column;
			++s;
			continue;
		}
		if ('{' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_OPEN_BRACE;
			json->token.u.string = NULL;
			return;
		}
		if ('}' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_CLOSE_BRACE;
			json->token.u.string = NULL;
			return;
		}
		if ('[' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_OPEN_BRACKET;
			json->token.u.string = NULL;
			return;
		}
		if (']' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_CLOSE_BRACKET;
			json->token.u.string = NULL;
			return;
		}
		if (':' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_COLON;
			json->token.u.string = NULL;
			return;
		}
		if (',' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_COMMA;
			json->token.u.string = NULL;
			return;
		}
		if (!strncmp("null", s, 4)) {
			json->curr = s + 4;
			json->column += 4;
			json->token.op = OP_NULL;
			json->token.u.string = NULL;
			return;
		}
		if (!strncmp("true", s, 4)) {
			json->curr = s + 4;
			json->column += 4;
			json->token.op = OP_BOOL;
			json->token.u.bool_ = 1;
			return;
		}
		if (!strncmp("false", s, 5)) {
			json->curr = s + 5;
			json->column += 5;
			json->token.op = OP_BOOL;
			json->token.u.bool_ = 0;
			return;
		}
		if ('\"' == (*s)) {
			if (!(e = eat_string(s))) {
				ERROR(json, "erroneous string");
				return;
			}
			(*e) = '\0';
			++e;
			json->curr = e;
			json->column += (e - s);
			json->token.op = OP_STRING;
			json->token.u.string = s + 1;
			return;
		}
		if (('-' == (*s)) ||
		    ('.' == (*s)) ||
		    isdigit((unsigned char)(*s))) {
			errno = 0;
			r = strtod(s, &e);
			if ((EINVAL == errno) || (ERANGE == errno)) {
				ERROR(json, "erroneous number");
				return;
			}
			json->curr = e;
			json->column += (e - s);
			json->token.op = OP_NUMBER;
			json->token.u.number = r;
			return;
		}
		ERROR(json, "erroneous character");
		return;
	}
	json->token.op = OP_EOF;
	json->token.u.string = NULL;
}

static int
match(const struct g_json *json, int op)
{
	if (op == (int)json->token.op) {
		return 1;
	}
	return 0;
}

static struct g_json_node *jarray(struct g_json *json);
static struct g_json_node *jobject(struct g_json *json);

static struct g_json_node *
jvalue(struct g_json *json)
{
	struct g_json_node *node;

	node = NULL;
	if (match(json, OP_NULL)) {
		MKN(json, node, G_JSON_NODE_OP_NULL);
		forward(json);
	}
	else if (match(json, OP_BOOL)) {
		MKN(json, node, G_JSON_NODE_OP_BOOL);
		node->u.bool_ = json->token.u.bool_;
		forward(json);
	}
	else if (match(json, OP_NUMBER)) {
		MKN(json, node, G_JSON_NODE_OP_NUMBER);
		node->u.number = json->token.u.number;
		forward(json);
	}
	else if (match(json, OP_STRING)) {
		MKN(json, node, G_JSON_NODE_OP_STRING);
		node->u.string = json->token.u.string;
		forward(json);
	}
	else if (match(json, OP_OPEN_BRACKET)) {
		if (!(node = jarray(json))) {
			ERROR(json, "erroneous array");
			return NULL;
		}
	}
	else if (match(json, OP_OPEN_BRACE)) {
		if (!(node = jobject(json))) {
			ERROR(json, "erroneous object");
			return NULL;
		}
	}
	return node;
}

static struct g_json_node *
jarray(struct g_json *json)
{
	struct g_json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (match(json, OP_OPEN_BRACKET)) {
		forward(json);
		MKN(json, node, G_JSON_NODE_OP_ARRAY);
		head = tail = node;
		if (!match(json, OP_CLOSE_BRACKET)) {
			for (;;) {
				mark = 0;
				if (!(node->u.array.node = jvalue(json))) {
					ERROR(json, "erroneous value");
					return NULL;
				}
				if (!match(json, OP_COMMA)) {
					break;
				}
				forward(json);
				MKN(json, node, G_JSON_NODE_OP_ARRAY);
				tail->u.array.link = node;
				tail = node;
				mark = 1;
			}
			if (mark) {
				ERROR(json, "dangling ','");
				return NULL;
			}
		}
		if (!match(json, OP_CLOSE_BRACKET)) {
			ERROR(json, "missing ']'");
			return NULL;
		}
		forward(json);
	}
	return head;
}

static struct g_json_node *
jobject(struct g_json *json)
{
	struct g_json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (match(json, OP_OPEN_BRACE)) {
		forward(json);
		MKN(json, node, G_JSON_NODE_OP_OBJECT);
		head = tail = node;
		for (;;) {
			if (!match(json, OP_STRING)) {
				break;
			}
			mark = 0;
			node->u.object.key = json->token.u.string;
			forward(json);
			if (!match(json, OP_COLON)) {
				ERROR(json, "missing ':'");
				return NULL;
			}
			forward(json);
			if (!(node->u.object.node = jvalue(json))) {
				ERROR(json, "erroneous value");
				return NULL;
			}
			if (!match(json, OP_COMMA)) {
				break;
			}
			forward(json);
			MKN(json, node, G_JSON_NODE_OP_OBJECT);
			tail->u.object.link = node;
			tail = node;
			mark = 1;
		}
		if (mark) {
			ERROR(json, "dangling ','");
			return NULL;
		}
		if (!match(json, OP_CLOSE_BRACE)) {
			ERROR(json, "missing '}'");
			return NULL;
		}
		forward(json);
	}
	return head;
}

static struct g_json_node *
jtop(struct g_json *json)
{
	struct g_json_node *node;

	node = NULL;
	if (match(json, OP_OPEN_BRACKET)) {
		if (!(node = jarray(json))) {
			ERROR(json, "erroneous array");
			return NULL;
		}
	}
	else if (match(json, OP_OPEN_BRACE)) {
		if (!(node = jobject(json))) {
			ERROR(json, "erroneous object");
			return NULL;
		}
	}
	return node;
}

g_json_t
g_json_open(const char *s)
{
	struct g_json *json;

	// initialize

	if (!(json = g_malloc(sizeof (struct g_json)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(json, 0, sizeof (struct g_json));
	if (!(json->content = g_strdup(s))) {
		g_json_close(json);
		G_TRACE("^");
		return NULL;
	}

	// initialize

	json->curr = json->content;
	json->lineno = 1;
	json->column = 1;
	forward(json);

	// parse

	if (!(json->root = jtop(json))) {
		ERROR(json, "empty translation unit");
		g_json_close(json);
		return NULL;
	}
	if (!match(json, OP_EOF)) {
		ERROR(json, "superfluous content");
		g_json_close(json);
		return NULL;
	}
	return json;
}

void
g_json_close(g_json_t json)
{
	struct chunk *chunk;

	if (json) {
		while ((chunk = json->chunk)) {
			json->chunk = chunk->link;
			g_free(chunk);
		}
		g_free(json->content);
		memset(json, 0, sizeof (struct g_json));
		g_free(json);
	}
}

const struct g_json_node *
g_json_root(g_json_t json)
{
	assert( json );

	return json->root;
}

int
g_json_array_size(const struct g_json_node *node)
{
	uint64_t size;

	assert( node && (G_JSON_NODE_OP_ARRAY == node->op) );

	size = 0;
	while (node) {
		size += node->u.array.node ? 1 : 0;
		node = node->u.array.link;
	}
	return size;
}
