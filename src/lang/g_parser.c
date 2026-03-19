// Copyright (c) Tony Givargis, 2024-2026

#include "g_parser.h"

#define NLEN ( sizeof (struct g_lang_node) )

#define ERROR(p, f, a)					\
	do {						\
		if (!(p)->stop) {			\
			g_log("error: "			\
			      "parser:%u:%u: " f,	\
			      next(p)->lineno,		\
			      next(p)->column,		\
			      (a));			\
			G_TRACE("parser error");	\
		}					\
		(p)->stop = 1;				\
	}						\
	while (0)

#define MKN(p, d, o)						\
	do {							\
		(d) = g_vector_append((p)->nodes, NLEN);	\
		if (!(d)) {					\
			ERROR((p), "out of memory", "");	\
			return NULL;				\
		}						\
		(d)->id = ++(p)->id;				\
		(d)->op = (o);					\
		(d)->token = next((p));				\
	}							\
	while (0)

struct g_parser {
	int id;
	int stop;
	uint64_t i; // current token
	uint64_t n; // total tokens
	g_map_t map;
	g_lexer_t lexer;
	g_vector_t nodes;
	struct g_lang_node *node;
};

static const struct g_lexer_token *
next(const struct g_parser *parser)
{
	if (!parser->stop && (parser->i < parser->n)) {
		return g_lexer_lookup(parser->lexer, parser->i);
	}
	return g_lexer_lookup(parser->lexer, parser->n - 1); // LEXER_END
}

static int
match(const struct g_parser *parser, int op)
{
	const struct g_lexer_token *token;

	if ((token = next(parser)) && (op == token->op)) {
		return 1;
	}
	return 0;
}

static void
forward(struct g_parser *parser)
{
	if (parser->i < parser->n) {
		++parser->i;
	}
}

/**============================================================================
 * (E0)  expr_list
 * (E1)  expr_primary
 * (E2)  expr_postfix
 * (E3)  expr_unary
 * (E4)  expr_multiplicative
 * (E5)  expr_additive
 * (E6)  expr_shift
 * (E7)  expr_relational
 * (E8)  expr_equality
 * (E9)  expr_and
 * (E10) expr_xor
 * (E11) expr_or
 * (E12) expr_logic_and
 * (E13) expr_logic_or
 * (E14) expr_cond
 * (E15) expr
 *===========================================================================*/

static struct g_lang_node *expr(struct g_parser *parser);

/**
 * (E0)
 *
 * expr_list : expr { ',' expr }
 */

static struct g_lang_node *
expr_list(struct g_parser *parser)
{
	struct g_lang_node *node, *node_, *head, *tail;
	int mark;

	mark = 0;
	head = tail = NULL;
	while ((node_ = expr(parser))) {
		MKN(parser, node, G_LANG_OP_EXPR_LIST);
		node->cond = node_;
		if (tail) {
			tail->right = node;
			tail = node;
		}
		else {
			head = node;
			tail = node;
		}
		if (!(mark = match(parser, G_LEXER_OPERATOR_COMMA))) {
			break;
		}
		forward(parser);
	}
	if (mark) {
		ERROR(parser, "dangling ','", "");
		return NULL;
	}
	return head;
}

/**
 * (E1)
 *
 * expr_primary :
 *              | INT
 *              | FLOAT
 *              | STRING
 *              | IDENTIFIER
 *              | '(' expr ')'
 */

static struct g_lang_node *
expr_primary(struct g_parser *parser)
{
	struct g_lang_node *node, *node_;

	node = NULL;
	if (match(parser, G_LEXER_INT)) {
		MKN(parser, node, G_LANG_OP_EXPR_INT);
		forward(parser);
	}
	else if (match(parser, G_LEXER_FLOAT)) {
		MKN(parser, node, G_LANG_OP_EXPR_FLOAT);
		forward(parser);
	}
	else if (match(parser, G_LEXER_STRING)) {
		MKN(parser, node, G_LANG_OP_EXPR_STRING);
		forward(parser);
	}
	else if (match(parser, G_LEXER_IDENTIFIER)) {
		if (!(node_ = g_map_lookup(parser->map, next(parser)->u.s))) {
			ERROR(parser,
			      "undefined symbol '%s'",
			      next(parser)->u.s);
			return NULL;
		}
		MKN(parser, node, G_LANG_OP_EXPR_IDENTIFIER);
		node->left = node_; // definition
		forward(parser);
	}
	else if (match(parser, G_LEXER_OPERATOR_LPAR)) {
		forward(parser);
		if (!(node = expr(parser))) {
			ERROR(parser, "invalid expression after '('", "");
			return NULL;
		}
		if (!match(parser, G_LEXER_OPERATOR_RPAR)) {
			ERROR(parser, "missing ')'", "");
			return NULL;
		}
		forward(parser);
	}
	return node;
}

/**
 * (E2)
 *
 * expr_postfix_ : { '[' expr ']' }
 *	         | { '(' expr_list ')' }
 *	         | { '.' IDENTIFIER }
 *	         | { [ '++' '--' ] }
 *               | <e>
 *
 * expr_postfix : expr_primary expr_postfix_
 */

static struct g_lang_node *
expr_postfix_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_LSQB)) {
			MKN(parser, node, G_LANG_OP_EXPR_ARRAY);
			node->left = left;
			forward(parser);
			if (!(node->right = expr(parser))) {
				ERROR(parser,
				      "missing array index expression",
				      "");
				return NULL;
			}
			if (!match(parser, G_LEXER_OPERATOR_RSQB)) {
				ERROR(parser, "missing ']'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, G_LEXER_OPERATOR_LPAR)) {
			MKN(parser, node, G_LANG_OP_EXPR_FUNCTION);
			node->left = left;
			forward(parser);
			node->right = expr_list(parser);
			if (!match(parser, G_LEXER_OPERATOR_RPAR)) {
				ERROR(parser, "missing ')'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, G_LEXER_OPERATOR_DOT)) {
			MKN(parser, node, G_LANG_OP_EXPR_FIELD);
			node->left = left;
			forward(parser);
			if (!match(parser, G_LEXER_IDENTIFIER)) {
				ERROR(parser, "missing identifier", "");
				return NULL;
			}
			node->token = next(parser);
			forward(parser);
		}
		else if (match(parser, G_LEXER_OPERATOR_INC)) {
			MKN(parser, node, G_LANG_OP_EXPR_INC);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, G_LEXER_OPERATOR_DEC)) {
			MKN(parser, node, G_LANG_OP_EXPR_DEC);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		left = node;
	}
	return node;

}

static struct g_lang_node *
expr_postfix(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_primary(parser))) {
		return NULL;
	}
	return expr_postfix_(parser, node);
}

/**
 * (E3)
 *
 * expr_unary : [ '+' '-' '~' '!' '++' '--' ] expr_unary
 *            | expr_postfix
 */

static struct g_lang_node *
expr_unary(struct g_parser *parser)
{
	struct g_lang_node *node;

	node = NULL;
	if (match(parser, G_LEXER_OPERATOR_ADD)) {
		forward(parser);
		if (!(node = expr_unary(parser))) {
			ERROR(parser, "invalid unary '+' operand", "");
			return NULL;
		}
	}
	else if (match(parser, G_LEXER_OPERATOR_SUB)) {
		MKN(parser, node, G_LANG_OP_EXPR_NEG);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			ERROR(parser, "invalid unary '-' operand", "");
			return NULL;
		}
	}
	else if (match(parser, G_LEXER_OPERATOR_NOT)) {
		MKN(parser, node, G_LANG_OP_EXPR_NOT);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			ERROR(parser, "invalid '~' operand", "");
			return NULL;
		}
	}
	else if (match(parser, G_LEXER_OPERATOR_LOGIC_NOT)) {
		MKN(parser, node, G_LANG_OP_EXPR_LOGIC_NOT);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			ERROR(parser, "invalid '!' operand", "");
			return NULL;
		}
	}
	else if (match(parser, G_LEXER_OPERATOR_INC)) {
		MKN(parser, node, G_LANG_OP_EXPR_INC);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			ERROR(parser, "invalid '++' operand", "");
			return NULL;
		}
	}
	else if (match(parser, G_LEXER_OPERATOR_DEC)) {
		MKN(parser, node, G_LANG_OP_EXPR_DEC);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			ERROR(parser, "invalid '--' operand", "");
			return NULL;
		}
	}
	else {
		node = expr_postfix(parser);
	}
	return node;
}

/**
 * (E4)
 *
 * expr_multiplicative_ : { [ '*' '/' '%' ] expr_unary expr_multiplicative_ }
 *                      | <e>
 *
 * expr_multiplicative : expr_unary expr_multiplicative_
 */

static struct g_lang_node *
expr_multiplicative_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_MUL)) {
			MKN(parser, node, G_LANG_OP_EXPR_MUL);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_unary(parser))) {
				ERROR(parser, "invalid '*' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_DIV)) {
			MKN(parser, node, G_LANG_OP_EXPR_DIV);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_unary(parser))) {
				ERROR(parser, "invalid '/' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_MOD)) {
			MKN(parser, node, G_LANG_OP_EXPR_MOD);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_unary(parser))) {
				ERROR(parser, "invalid '%%' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_multiplicative(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_unary(parser))) {
		return NULL;
	}
	return expr_multiplicative_(parser, node);
}

/**
 * (E5)
 *
 * expr_additive_ : { [ '+' '-' ] expr_multiplicative expr_additive_ }
 *                | <e>
 *
 * expr_additive : expr_multiplicative expr_additive_
 */

static struct g_lang_node *
expr_additive_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_ADD)) {
			MKN(parser, node, G_LANG_OP_EXPR_ADD);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_multiplicative(parser))) {
				ERROR(parser, "invalid '+' operand", "");
				return NULL;
			}
			if (!(node = expr_additive_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_SUB)) {
			MKN(parser, node, G_LANG_OP_EXPR_SUB);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_multiplicative(parser))) {
				ERROR(parser, "invalid '-' operand", "");
				return NULL;
			}
			if (!(node = expr_additive_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_additive(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_multiplicative(parser))) {
		return NULL;
	}
	return expr_additive_(parser, node);
}

/**
 * (E6)
 *
 * expr_shift_ : { [ '<<' '>>' ] expr_additive expr_shift_ }
 *             | <e>
 *
 * expr_shift : expr_additive expr_shift_
 */

static struct g_lang_node *
expr_shift_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_SHL)) {
			MKN(parser, node, G_LANG_OP_EXPR_SHL);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_additive(parser))) {
				ERROR(parser, "invalid '<<' operand", "");
				return NULL;
			}
			if (!(node = expr_shift_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_SHR)) {
			MKN(parser, node, G_LANG_OP_EXPR_SHR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_additive(parser))) {
				ERROR(parser, "invalid '>>' operand", "");
				return NULL;
			}
			if (!(node = expr_shift_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_shift(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_additive(parser))) {
		return NULL;
	}
	return expr_shift_(parser, node);
}

/**
 * (E7)
 *
 * expr_relational_ : { [ '<' '>' '<=' '>=' ] expr_shift expr_relational_ }
 *                  | <e>
 *
 * expr_relational : expr_shift expr_relational_
 */

static struct g_lang_node *
expr_relational_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_LT)) {
			MKN(parser, node, G_LANG_OP_EXPR_LT);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '<' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_GT)) {
			MKN(parser, node, G_LANG_OP_EXPR_GT);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '>' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_LE)) {
			MKN(parser, node, G_LANG_OP_EXPR_LE);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '<=' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (match(parser, G_LEXER_OPERATOR_GE)) {
			MKN(parser, node, G_LANG_OP_EXPR_GE);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '>=' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_relational(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_shift(parser))) {
		return NULL;
	}
	return expr_relational_(parser, node);
}

/**
 * (E8)
 *
 * expr_equality_ : { [ '==' '!=' ] expr_relational expr_equality_ }
 *                | <e>
 *
 * expr_equality : expr_relational expr_equality_
 */

static struct g_lang_node *
expr_equality_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_EQ)) {
			MKN(parser, node, G_LANG_OP_EXPR_EQ);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, G_LEXER_OPERATOR_NE)) {
			MKN(parser, node, G_LANG_OP_EXPR_NE);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_relational(parser))) {
			ERROR(parser,
			      "invalid '%s' operand",
			      (node->op == G_LANG_OP_EXPR_EQ) ? "==" : "!=");
			return NULL;
		}
		if (!(node = expr_equality_(parser, node))) {
			G_TRACE("^");
			return NULL;
		}
	}
	return node;
}

static struct g_lang_node *
expr_equality(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_relational(parser))) {
		return NULL;
	}
	return expr_equality_(parser, node);
}

/**
 * (E9)
 *
 * expr_and_ : { '&' expr_equality expr_and_ }
 *           | <e>
 *
 * expr_and : expr_equality expr_and_
 */

static struct g_lang_node *
expr_and_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_AND)) {
			MKN(parser, node, G_LANG_OP_EXPR_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_equality(parser))) {
				ERROR(parser, "invalid '&' operand", "");
				return NULL;
			}
			if (!(node = expr_and_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_and(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_equality(parser))) {
		return NULL;
	}
	return expr_and_(parser, node);
}

/**
 * (E10)
 *
 * expr_xor_ : { '^' expr_and expr_xor_ }
 *           | <e>
 *
 * expr_xor : expr_and expr_xor_
 */

static struct g_lang_node *
expr_xor_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_XOR)) {
			MKN(parser, node, G_LANG_OP_EXPR_XOR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_and(parser))) {
				ERROR(parser, "invalid '^' operand", "");
				return NULL;
			}
			if (!(node = expr_xor_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_xor(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_and(parser))) {
		return NULL;
	}
	return expr_xor_(parser, node);
}

/**
 * (E11)
 *
 * expr_or_ : { '|' expr_xor expr_or_ }
 *          | <e>
 *
 * expr_or : expr_xor expr_or_
 */

static struct g_lang_node *
expr_or_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_OR)) {
			MKN(parser, node, G_LANG_OP_EXPR_OR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_xor(parser))) {
				ERROR(parser, "invalid '|' operand", "");
				return NULL;
			}
			if (!(node = expr_or_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_or(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_xor(parser))) {
		return NULL;
	}
	return expr_or_(parser, node);
}

/**
 * (E12)
 *
 * expr_logic_and_ : { '&&' expr_or expr_logic_and_ }
 *                 | <e>
 *
 * expr_logic_and : expr_or expr_logic_and_
 */

static struct g_lang_node *
expr_logic_and_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_LOGIC_AND)) {
			MKN(parser, node, G_LANG_OP_EXPR_LOGIC_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_or(parser))) {
				ERROR(parser, "invalid '&&' operand", "");
				return NULL;
			}
			if (!(node = expr_logic_and_(parser, node))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct g_lang_node *
expr_logic_and(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_or(parser))) {
		return NULL;
	}
	return expr_logic_and_(parser, node);
}

/**
 * (E13)
 *
 * expr_logic_or_ : { '||' expr_logic_and expr_logic_or_ }
 *                | <e>
 *
 * expr_logic_or : expr_logic_and expr_logic_or_
 */

static struct g_lang_node *
expr_logic_or_(struct g_parser *parser, struct g_lang_node *left)
{
	struct g_lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, G_LEXER_OPERATOR_LOGIC_OR)) {
			MKN(parser, node, G_LANG_OP_EXPR_LOGIC_OR);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_logic_and(parser))) {
			ERROR(parser, "invalid '||' operand", "");
			return NULL;
		}
		if (!(node = expr_logic_or_(parser, node))) {
			G_TRACE("^");
			return NULL;
		}
	}
	return node;
}

static struct g_lang_node *
expr_logic_or(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr_logic_and(parser))) {
		return NULL;
	}
	return expr_logic_or_(parser, node);
}

/**
 * (E14)
 *
 * expr : expr_logic_or
 *      | expr_logic_or '?' expr ':' expr
 */

static struct g_lang_node *
expr_cond(struct g_parser *parser)
{
	struct g_lang_node *node, *node_;

	if (!(node = expr_logic_or(parser))) {
		return NULL;
	}
	if (match(parser, G_LEXER_OPERATOR_QUESTION)) {
		MKN(parser, node_, G_LANG_OP_EXPR_COND);
		node_->cond = node;
		forward(parser);
		if (!(node_->left = expr(parser))) {
			ERROR(parser, "invalid '?' operand", "");
			return NULL;
		}
		if (!match(parser, G_LEXER_OPERATOR_COLON)) {
			ERROR(parser, "invalid ':'", "");
			return NULL;
		}
		forward(parser);
		if (!(node_->right = expr(parser))) {
			ERROR(parser, "invalid ':' operand", "");
			return NULL;
		}
		node = node_;
	}
	return node;
}

/**
 * (E15)
 *
 * top : expr
 */

static struct g_lang_node *
expr(struct g_parser *parser)
{
	return expr_cond(parser);
}

/*---------------------------------------------------------------------------*/
/* END OF GRAMMAR */
/*---------------------------------------------------------------------------*/

static struct g_lang_node *
top(struct g_parser *parser)
{
	struct g_lang_node *node;

	if (!(node = expr(parser))) {
		ERROR(parser, "invalid program", "");
		return NULL;
	}
	if (!match(parser, G_LEXER_END)) {
		ERROR(parser, "invalid token", "");
		return NULL;
	}
	return node;
}

g_parser_t
g_parser_open(const char *pathname)
{
	struct g_parser *parser;

	assert( pathname && (*pathname) );

	if (!(parser = g_malloc(sizeof (struct g_parser)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(parser, 0, sizeof (struct g_parser));
	if (!(parser->nodes = g_vector_open()) ||
	    !(parser->lexer = g_lexer_open(pathname)) ||
	    !(parser->map = g_map_open())) {
		g_parser_close(parser);
		G_TRACE("^");
		return NULL;
	}
	if (1 >= (parser->n = g_lexer_items(parser->lexer))) {
		ERROR(parser, "empty translation unit", "");
		g_parser_close(parser);
		return NULL;
	}
	if (!(parser->node = top(parser))) {
		g_parser_close(parser);
		G_TRACE("^");
		return NULL;
	}
	return parser;
}

void
g_parser_close(g_parser_t parser)
{
	if (parser) {
		g_map_close(parser->map);
		g_lexer_close(parser->lexer);
		g_vector_close(parser->nodes);
		memset(parser, 0, sizeof (struct g_parser));
		g_free(parser);
	}
}

struct g_lang_node *
g_parser_root(g_parser_t parser)
{
	assert( parser );

	return parser->node;
}
