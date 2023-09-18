#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <enet/enet.h>

#include <comms.h>

#define MAX_CONNECTIONS 64

static struct CommsEvent comms_nothing = {COMMS_NOTHING, NULL, 0, 0};

struct ComServer {
	ENetHost * host;
};

struct Peer {
	ENetHost * host;
	ENetPeer * peer;
	ENetPacket * packet;
	bool clientOnly;
};

int sendMessageToPeer(Peer *peer, unsigned char *data, int datasize){

	ENetPacket * packet = enet_packet_create(data, datasize, ENET_PACKET_FLAG_RELIABLE);

	enet_peer_send(peer->peer, 0, packet);

	enet_host_flush(peer->host);

	return datasize;
	
}

int getMessageFromPeer(Peer *peer, unsigned char *buf, int bufsize){
	
	int datasize = peer->packet->dataLength;

	int n = bufsize < datasize ? bufsize : datasize;

	memcpy(buf, peer->packet->data, n);

	enet_packet_destroy(peer->packet);

	peer->packet = NULL;

	return n;
}

struct CommsEvent getServerEvent(ComServer *server){

	ENetEvent event;

	if (enet_host_service(server->host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			Peer * peer = mallocPeer();
			peer->peer = event->peer;
			peer->host = server;
			peer->clientOnly = false;
			event->peer->data = peer; // cyclic reference
			struct CommsEvent ev = {COMMS_CONNECT, peer, 0, 0};
			return ev;
		case ENET_EVENT_TYPE_RECEIVE:
			Peer * peer = event.peer->data;
			peer->packet = event.packet;
			struct CommsEvent ev = {COMMS_MESSAGE, peer, event.packet.dataLength, 0};
			return ev;
		case ENET_EVENT_TYPE_DISCONNECT:
			Peer * peer = event.peer->data;
			struct CommsEvent ev = {COMMS_DISCONNECT, peer, 0, 0};
			return ev;
		}
	}

	return comms_nothing;

}

struct CommsEvent getPeerEvent(Peer *them){

	ENetEvent event;

	int result = enet_host_service(them->host, &event, 0);

	if (result == 0) return comms_nothing;

	if (result < 0) {
		fprintf(stderr, "enet_host_service: failed\n");
		struct CommsEvent ev = {COMMS_ERROR, NULL, 0, result};
		return ev;
	}

	switch (event.type) {
	case ENET_EVENT_TYPE_CONNECT:
		//connection complete
		struct CommsEvent ev = {COMMS_CONNECT, them, 0, 0};
	case ENET_EVENT_TYPE_RECEIVE:
		them->packet = event.packet;
		struct CommsEvent ev = {COMMS_MESSAGE, them, event.packet.dataLength, 0};
		return ev;
	case ENET_EVENT_TYPE_DISCONNECT:
		enet_peer_reset(them->peer);
		//the host has served its purpose
		enet_host_destroy(them->host);
		them->host = NULL;
		them->peer = NULL;
		them->packet = NULL;
		struct CommsEvent ev = {COMMS_DISCONNECT, them, 0, 0};
		return ev;
	default:
		fprintf(stderr, "enet_host_service: bad event type\n");
		struct CommsEvent ev = {COMMS_ERROR, NULL, 0, event.type};
		return ev;
	}

}


Peer * connectToRemoteHost(const char *hostname, int port){

	ENetAddress address;

	int status = enet_address_set_host(&address, hostname);
	if (status < 0) {
		fprintf(stderr, "enet_address_set_host: failed to resolve hostname\n");
		return NULL;
	}

	address.port = port;

	ENetHost *host = enet_host_create(NULL, 1, 1, 0, 0);

	if (host == NULL) {
		fprintf(stderr, "enet_host_create: failed\n");
		return NULL;
	}

	ENetPeer *peer = enet_host_connect(host, &address, 1, 0);

	if (peer == NULL) {
		fprintf(stderr, "enet_host_connect: failed\n");
		return NULL;
	}

	struct Peer *them = mallocPeer();
	them->host = host;
	them->peer = peer;
	them->clientOnly = true;
	peer->data = them; // cyclic reference

	// connection not complete until a CONNECTION event is detected
	
	return them;
	
}

void disconnectPeer(Peer *peer){

	if(peer->clientOnly){
		enet_reset_peer(peer->peer);
		enet_host_destroy(peer->host);
		free(peer);
	}
	else{
		enet_peer_disconnect(peer->peer, 0);
	}

}



ComServer * spawnServer(int port){

	ENetAddress address;

	address.host = ENET_HOST_ANY;
	address.port = port;

	ENetHost *host = enet_host_create(&address, MAX_CONNECTIONS, 1, 0, 0);

	if (host == NULL) {
		fprintf(stderr, "enet_host_create: failed\n");
		return NULL;
	}

	ComServer * server = mallocServer();

	server->host = host;

	return server;

}


void unspawnServer(ComServer *server){
	enet_host_destroy(server->host);
	free(server);
}
