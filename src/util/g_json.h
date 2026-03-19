// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_JSON_H_
#define _G_JSON_H_

#include "g_core.h"

struct g_json_node {
	enum g_json_node_op {
		G_JSON_NODE_OP_,
		G_JSON_NODE_OP_NULL,
		G_JSON_NODE_OP_BOOL,
		G_JSON_NODE_OP_ARRAY,
		G_JSON_NODE_OP_OBJECT,
		G_JSON_NODE_OP_NUMBER,
		G_JSON_NODE_OP_STRING
	} op;
	union {
		int bool_;
		double number;
		const char *string;
		struct g_json_node_array {
			struct g_json_node *node;
			struct g_json_node *link;
		} array;
		struct g_json_node_object {
			const char *key;
			struct g_json_node *node;
			struct g_json_node *link;
		} object;
	} u;
};

typedef struct g_json *g_json_t;

g_json_t g_json_open(const char *s);

void g_json_close(g_json_t json);

const struct g_json_node *g_json_root(g_json_t json);

int g_json_array_size(const struct g_json_node *node);

#endif
