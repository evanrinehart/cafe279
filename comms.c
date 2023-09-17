#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>

#include <comms.h>

#define MAX_CONNECTIONS 64
#define MAILBOX_SIZE 1024
#define LARGE_MAILBOX_SIZE (1 << 16)

struct ComServer {
	int socket;
	int port;
	Peer * peers[MAX_CONNECTIONS];
	Peer **peers_ptr;
	Peer **peers_end;
	int datasize;
	unsigned char data[LARGE_MAILBOX_SIZE];
};

struct Peer {
	int socket;
	struct sockaddr_storage addr;
	socklen_t addrlen;
	time_t deathtime;
	bool clientOnly;
	ComServer *server;
	int datasize;
	unsigned char data[MAILBOX_SIZE];
};

static ComServer * mallocServer(){
	ComServer * server = malloc(sizeof (struct ComServer));
	server->peers_ptr = server->peers;
	server->peers_end = server->peers + MAX_CONNECTIONS;
	return server;
}

static Peer * mallocPeer(){
	Peer * peer = malloc(sizeof (struct Peer));
	peer->datasize = 0;
	return peer;
}

int sendMessageToPeer(Peer *peer, unsigned char *data, int datasize){
	
	int num_written = sendto(
		peer->socket,
		data,
		datasize,
		0,
		(struct sockaddr*)&peer->addr,
		peer->addrlen
	);

	if(num_written == EAGAIN || num_written == EWOULDBLOCK){
		return 0;
	}

	if (num_written < 0) {
		// EBADF
		// EFAULT
		// EINTR - signal occurred before data was transmitted
		// EMSGSIZE - message too big
		// ENOTSOCK
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		return -1;
	}

	if (num_written < datasize) {
		fprintf(stderr, "sendto sent fewer bytes than requested %d / %d\n", num_written, datasize);
	}

	return num_written;
}

int getMessageFromPeer(Peer *peer, unsigned char *buf, int bufsize){
	int n = bufsize < peer->datasize ? bufsize : peer->datasize;
	memcpy(buf, peer->data, n);
	return peer->datasize;
}

static Peer * maybeExpirePeer(ComServer * server){
	// FIXME
	return NULL;
}

struct CommsEvent getServerEvent(ComServer *server){

	Peer * peer = maybeExpirePeer(server);

	if(peer){
		struct CommsEvent ev = {COMMS_DISCONNECT, peer, 0, 0};
		return ev;
	}

	struct sockaddr_storage addr;
	socklen_t addrlen;
	
	int num_read = recvfrom(
		server->socket,
		server->data,
		sizeof server->data,
		0,
		(struct sockaddr*)&addr,
		&addrlen
	);

	if(num_read == EAGAIN || num_read == EWOULDBLOCK){
		struct CommsEvent ev = {COMMS_NOTHING, NULL, 0, 0};
		return ev;
	}

	if(num_read < 0){
		fprintf(stderr,"recvfrom: %s\n", strerror(errno));
		struct CommsEvent ev = {COMMS_ERROR, NULL, 0, errno};
		return ev;
	}

	// FIXME bogus

//	m->datasize = num_read;

//	struct CommsEvent ev = {COMMS_MESSAGE, peer, 0, 0};
//	return ev;z

	// see if any peers have expired, return DISCONNECT event

	// otherwise recvfrom

	// if unrecognized peer, and they have the right message, 
	// and we have room left
	// generate a new peer and CONNECT event
	// if no room left, silently ignore, return NOTHING

	// if recognized peer, copy message into peer storage
	// and generate MESSAGE event

	// if no message, NOTHING

}

struct CommsEvent getPeerEvent(Peer *peer){
	
	int num_read = recv(
		peer->socket,
		peer->data,
		sizeof peer->data,
		0
	);

	if(num_read == ECONNREFUSED){
		puts("recv: ECONNREFUSED");
		struct CommsEvent ev = {COMMS_DISCONNECT, peer, 0, 0}; return ev;
	}

	if(num_read == EAGAIN || num_read == EWOULDBLOCK){
		struct CommsEvent ev = {COMMS_NOTHING, peer, 0, 0}; return ev;
	}

	if(num_read < 0){
		fprintf(stderr,"recv: %s\n", strerror(errno));
		struct CommsEvent ev = {COMMS_ERROR, peer, 0, errno}; return ev;
	}

	peer->datasize = num_read;

	struct CommsEvent ev = {COMMS_MESSAGE, peer, num_read, 0};
	return ev;

}


Peer * connectToRemoteHost(const char *hostname, int port){

	struct addrinfo *res;
	struct addrinfo hints;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char service[64]; sprintf(service, "%d", port);

	int err = getaddrinfo(hostname, service, &hints, &res);

	if(err){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return NULL;
	}

	for (struct addrinfo *ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		int s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s < 0) {
			fprintf(stderr, "socket: %s\n", strerror(errno));
			continue;
		}

		err = connect(s, ptr->ai_addr, ptr->ai_addrlen);
		
		if (err < 0) {
			fprintf(stderr, "connect: %s\n", strerror(errno));
			close(s);
			continue;
		}

		err = fcntl(s, F_SETFL, O_NONBLOCK);
		if(err < 0){
			fprintf(stderr, "fcntl: %s\n", strerror(errno));
			return NULL;
		}

		Peer * peer = mallocPeer();
		peer->clientOnly = true;
		peer->socket = s;
		peer->datasize = 0;
		memcpy(&peer->addr, ptr->ai_addr, ptr->ai_addrlen);
		peer->addrlen = ptr->ai_addrlen;

		freeaddrinfo(res);

		return peer;
	}

	freeaddrinfo(res);

	fprintf(stderr, "getaddrinfo: could not bind socket\n");

	return NULL;
	
}

void disconnectPeer(Peer *peer){
	char msg[] = "GOODBYE\n";
	sendMessageToPeer(peer, msg, sizeof msg);

	// standalone peer
	if (peer->clientOnly) {
		close(peer->socket);
		free(peer);
		return;
	}

	// server managed peer
	Peer ** peers     = peer->server->peers;
	Peer ** peers_ptr = peer->server->peers_ptr;
	Peer ** peers_end = peer->server->peers_end;

	for (Peer **pptr = peers; pptr < peers_ptr; pptr++){
		if(*pptr != peer) continue;
		*pptr = *peers_ptr;
		peer->server->peers_ptr--;
	}

	free(peer);
}



ComServer * spawnServer(int port){
	ComServer * server = mallocServer();

	struct addrinfo *res;
	struct addrinfo hints;

	hints.ai_family = AF_UNSPEC;
	//hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char service[64]; sprintf(service, "%d", port);

	int err = getaddrinfo(NULL, service, &hints, &res);

	if(err){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		free(server);
		return NULL;
	}

	for (struct addrinfo *ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		int s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s < 0) {
			fprintf(stderr, "socket: %s\n", strerror(errno));
			continue;
		}

		err = bind(s, ptr->ai_addr, ptr->ai_addrlen);
		
		if (err < 0) {
			fprintf(stderr, "bind: %s\n", strerror(errno));
			close(s);
			continue;
		}

		server->socket = s;
		server->port = port;
		server->datasize = 0;

		freeaddrinfo(res);

		return server;
	}

	free(server);
	freeaddrinfo(res);

	fprintf(stderr, "getaddrinfo: no suitable server address\n");

	return NULL;
}


void unspawnServer(ComServer *server){
	puts("unspawnServer");

	Peer ** peers = server->peers;

	for(struct Peer **pptr = peers; pptr < server->peers_ptr; pptr++){
		puts("free peer");
		free(*pptr);
	}

	close(server->socket);

	free(server);
}
