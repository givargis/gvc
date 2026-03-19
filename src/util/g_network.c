// Copyright (c) Tony Givargis, 2024-2026

#define _POSIX_C_SOURCE 200809L

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netdb.h>

#include "g_thread.h"
#include "g_network.h"

#define CLOSE(fd)					\
	do {						\
		int errno_ = errno;			\
		if (0 < (fd)) {				\
			shutdown((fd), SHUT_RDWR);	\
			close(fd);			\
			(fd) = 0;			\
		}					\
		errno = errno_;				\
	}						\
	while (0)

struct g_network {
	int fd;
	struct server {
		int fd;
		g_thread_t thread;
		struct server *link;
		struct client {
			int fd;
			g_thread_t thread;
			struct client *link;
			//---
			void *ctx;
			g_network_fnc_t fnc;
		} *clients;
		//---
		void *ctx;
		g_network_fnc_t fnc;
	} *servers;
};

static int
osocket(int domain, int type, int protocol)
{
	const int NODELAY = 1;
	const int OPTVAL = 1;
	int fd;

	if ((0 >= (fd = socket(domain, type, protocol))) ||
	    (0 > setsockopt(fd,
			    IPPROTO_TCP,
			    TCP_NODELAY,
			    (const void *)&NODELAY,
			    sizeof (NODELAY))) ||
	    (0 > setsockopt(fd,
			    SOL_SOCKET,
			    SO_REUSEADDR,
			    (const void *)&OPTVAL,
			    sizeof (OPTVAL)))) {
		CLOSE(fd);
		return 0;
	}
	return fd;
}

static void
_client_(void *ctx)
{
	struct g_network network;
	struct client *client;

	client = (struct client *)ctx;
	memset(&network, 0, sizeof (struct g_network));
	network.fd = client->fd;
	client->fnc(client->ctx, &network);
	CLOSE(network.fd);
	client->fd = 0;
	client->ctx = NULL;
	client->fnc = NULL;
}

static void
_server_(void *ctx)
{
	struct server *server;
	struct client *client;
	int fd;

	server = (struct server *)ctx;
	while (__sync_fetch_and_add(&server->fd, 0)) {
		if (0 >= (fd = accept(server->fd, NULL, NULL))) {
			continue;
		}
		client = server->clients;
		while (client && g_thread_good(client->thread)) {
			client = client->link;
		}
		if (!client) {
			if (!(client = g_malloc(sizeof (struct client)))) {
				CLOSE(fd);
				G_TRACE("^ (ignored)");
				continue;
			}
			memset(client, 0, sizeof (struct client));
			client->link = server->clients;
			server->clients = client;
		}
		client->fd = fd;
		client->ctx = server->ctx;
		client->fnc = server->fnc;
		if (!(client->thread = g_thread_open(_client_, client))) {
			CLOSE(fd);
			client->fd = 0;
			client->ctx = NULL;
			client->fnc = NULL;
			G_TRACE("^ (ignored)");
		}
	}
}

g_network_t
g_network_server(const char *hostname,
		 const char *servname,
		 g_network_fnc_t fnc,
		 void *ctx)
{
	struct addrinfo hints, *res, *p;
	struct g_network *network;
	struct server *server;
	int fd;

	assert( hostname && (*hostname) );
	assert( hostname && (*servname) );
	assert( fnc );

	// initialize

	if (!(network = g_malloc(sizeof (struct g_network)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(network, 0, sizeof (struct g_network));

	// address

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		g_network_close(network);
		G_TRACE("invalid network address");
		return NULL;
	}

	// listen

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > bind(fd, p->ai_addr, p->ai_addrlen)) ||
		    (0 > listen(fd, SOMAXCONN))) {
			CLOSE(fd);
			p = p->ai_next;
			continue;
		}
		if (!(server = g_malloc(sizeof (struct server)))) {
			CLOSE(fd);
			g_network_close(network);
			G_TRACE("^");
			return NULL;
		}
		memset(server, 0, sizeof (struct server));
		server->fd = fd;
		server->ctx = ctx;
		server->fnc = fnc;
		if (!(server->thread = g_thread_open(_server_, server))) {
			CLOSE(fd);
			g_free(server);
			g_network_close(network);
			G_TRACE("^");
			return NULL;
		}
		server->link = network->servers;
		network->servers = server;
		p = p->ai_next;
	}
	freeaddrinfo(res);
	p = res = NULL;

	// listening?

	if (!network->servers) {
		g_network_close(network);
		G_TRACE("no network interface found");
		return NULL;
	}
	return network;
}

g_network_t
g_network_connect(const char *hostname, const char *servname)
{
	struct addrinfo hints, *res, *p;
	struct g_network *network;
	int fd;

	assert( hostname && (*hostname) );
	assert( hostname && (*servname) );

	// initialize

	if (!(network = g_malloc(sizeof (struct g_network)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(network, 0, sizeof (struct g_network));

	// address

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, servname, &hints, &res)) {
		g_network_close(network);
		G_TRACE("invalid network address");
		return NULL;
	}

	// open

	fd = 0;
	p = res;
	while (p) {
		if (!(fd = osocket(p->ai_family,
				   p->ai_socktype,
				   p->ai_protocol)) ||
		    (0 > connect(fd, p->ai_addr, p->ai_addrlen))) {
			CLOSE(fd);
			p = p->ai_next;
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	p = res = NULL;

	// connected?

	if (!fd) {
		g_network_close(network);
		G_TRACE("unable to connect to network");
		return NULL;
	}
	network->fd = fd;
	return network;
}

void
g_network_close(g_network_t network)
{
	struct server *server, *server_;
	struct client *client, *client_;

	if (network) {
		CLOSE(network->fd);
		server = network->servers;
		while (server) {
			server_ = server;
			server = server->link;
			CLOSE(server_->fd);
			g_thread_close(server_->thread);
			client = server_->clients;
			while (client) {
				client_ = client;
				client = client->link;
				g_thread_close(client_->thread);
				memset(client_, 0, sizeof (struct client));
				g_free(client_);
			}
			memset(server_, 0, sizeof (struct server));
			g_free(server_);
		}
		memset(network, 0, sizeof (struct g_network));
		g_free(network);
	}
}

int
g_network_read(g_network_t network, void *buf_, size_t len)
{
	char *buf = (char *)buf_;
	ssize_t n;

	assert( network && (!len || buf) );

	while (len) {
		if (0 >= (n = read(network->fd, buf, len))) {
			G_TRACE("network read failed");
			return -1;
		}
		buf += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}

int
g_network_write(g_network_t network, const void *buf_, size_t len)
{
	const char *buf = (const char *)buf_;
	ssize_t n;

	assert( network && (!len || buf) );

	while (len) {
		if (0 >= (n = write(network->fd, buf, len))) {
			G_TRACE("network write failed");
			return -1;
		}
		buf += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}

int
g_network_is_valid(const char *address)
{
	struct in6_addr ipv6;
	struct in_addr ipv4;

	if ((1 == inet_pton(AF_INET, address, &ipv4)) ||
	    (1 == inet_pton(AF_INET6, address, &ipv6))) {
		return 1;
	}
	return 0;
}
