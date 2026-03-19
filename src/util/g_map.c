// Copyright (c) Tony Givargis, 2024-2026

#include "g_map.h"

struct g_map {
	uint64_t items;
	struct node {
		int depth;
		char *key;
		const void *val; // caller managed
		struct node *left;
		struct node *right;
	} *root;
};

static int
delta(const struct node *node)
{
	return node ? node->depth : -1;
}

static int
balance(const struct node *node)
{
	return delta(node->left) - delta(node->right);
}

static int
depth(const struct node *a, const struct node *b)
{
	return (delta(a) > delta(b)) ? (delta(a) + 1) : (delta(b) + 1);
}

static struct node *
rotate_right(struct node *node)
{
	struct node *root;

	root = node->left;
	node->left = root->right;
	root->right = node;
	node->depth = depth(node->left, node->right);
	root->depth = depth(root->left, node);
	return root;
}

static struct node *
rotate_left(struct node *node)
{
	struct node *root;

	root = node->right;
	node->right = root->left;
	root->left = node;
	node->depth = depth(node->left, node->right);
	root->depth = depth(root->left, node);
	return root;
}

static struct node *
rotate_left_right(struct node *node)
{
	node->left = rotate_left(node->left);
	return rotate_right(node);
}

static struct node *
rotate_right_left(struct node *node)
{
	node->right = rotate_right(node->right);
	return rotate_left(node);
}

static void
destroy(struct node *root)
{
	if (root) {
		destroy(root->left);
		destroy(root->right);
		g_free(root->key);
		memset(root, 0, sizeof (struct node));
		g_free(root);
	}
}

static struct node *
update(struct g_map *map, struct node *root, const char *key, const void *val)
{
	struct node *node;
	int d;

	if (!root) {
		if (!(root = g_malloc(sizeof (struct node)))) {
			G_TRACE("^");
			return NULL;
		}
		memset(root, 0, sizeof (struct node));
		if (!(root->key = g_strdup(key))) {
			g_free(root);
			G_TRACE("^");
			return NULL;
		}
		root->val = val;
		++map->items;
		return root;
	}
	if (!(d = strcmp(key, root->key))) {
		root->val = val;
		return root;
	}
	if (0 > d) {
		if (!(node = update(map, root->left, key, val))) {
			G_TRACE("^");
			return NULL;
		}
		root->left = node;
		if (1 < balance(root)) {
			if (0 <= balance(root->left)) {
				root = rotate_right(root);
			}
			else {
				root = rotate_left_right(root);
			}
		}
	}
	else {
		if (!(node = update(map, root->right, key, val))) {
			G_TRACE("^");
			return NULL;
		}
		root->right = node;
		if (-1 > balance(root)) {
			if (0 >= balance(root->right)) {
				root = rotate_left(root);
			}
			else {
				root = rotate_right_left(root);
			}
		}
	}
	root->depth = depth(root->left, root->right);
	return root;
}

static int
iterate(struct node *root, g_map_fnc_t fnc, void *ctx)
{
	int e;

	if (root) {
		if ((e = iterate(root->left, fnc, ctx)) ||
		    (e = fnc(ctx, root->key, (void *)root->val)) ||
		    (e = iterate(root->right, fnc, ctx))) {
			return e;
		}
	}
	return 0;
}

g_map_t
g_map_open(void)
{
	struct g_map *map;

	if (!(map = g_malloc(sizeof (struct g_map)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(map, 0, sizeof (struct g_map));
	return map;
}

void
g_map_close(g_map_t map)
{
	if (map) {
		destroy(map->root);
		memset(map, 0, sizeof (struct g_map));
		g_free(map);
	}
}

void
g_map_empty(g_map_t map)
{
	if (map) {
		destroy(map->root);
		memset(map, 0, sizeof (struct g_map));
	}
}

int
g_map_update(g_map_t map, const char *key, const void *val)
{
	struct node *root;

	assert( map && key && (*key) );

	if (!(root = update(map, map->root, key, val))) {
		G_TRACE("^");
		return -1;
	}
	map->root = root;
	return 0;
}

void *
g_map_lookup(g_map_t map, const char *key)
{
	const struct node *node;
	int d;

	assert( map && key && (*key) );

	node = map->root;
	while (node) {
		if (!(d = strcmp(key, node->key))) {
			return (void *)node->val;
		}
		node = (0 > d) ? node->left : node->right;
	}
	return NULL;
}

int
g_map_iterate(g_map_t map, g_map_fnc_t fnc, void *ctx)
{
	int e;

	assert( map && fnc );

	if ((e = iterate(map->root, fnc, ctx))) {
		return e;
	}
	return 0;
}

uint64_t
g_map_items(g_map_t map)
{
	assert( map );

	return map->items;
}
