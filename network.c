#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//#include <time.h>

#include <network.h>

#include <enet/enet.h>

#define MAX_CONNECTIONS 64
#define MAX_IDENTIFIER 64

static bool initCalled = false;

struct Server {
	bool enabled;

	ENetHost *host;

	struct NetworkCallbacks1 callbacks;
};

static struct Server server;

struct Client {
	bool enabled;
	bool connected;

	ENetHost *host;
	ENetPeer *peer;

	int chunksLeft;
	int chunkCounter;
	double abortTime;

	struct NetworkCallbacks2 callbacks;
};

static struct Client client;

struct Conn {
	int active;
	ENetPeer *peer;
	char identifier[64];
};

static struct Conn conns[MAX_CONNECTIONS];

static int getUnusedConnId(){

	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (conns[i].active) continue;
		conns[i].active = 1;
		return i;
	}

	return -1;
}

static int ensureEnetInitialized() {
    if (initCalled) return 0;

    int status = enet_initialize();

    if (status < 0) {
        fprintf(stderr, "enet_initialize: failed\n");
        return -1; 
    }   

    for(int i = 0; i < MAX_CONNECTIONS; i++){
        conns[i].active = 0;
        conns[i].peer = NULL;
        conns[i].identifier[0] = 0;
    }

    client.enabled = false;
    client.connected = false;
    client.peer = NULL;
    client.host = NULL;
    client.chunksLeft = 0;

    initCalled = true;

    return 0;
}


/*
static double currentTime(){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return 1e9 * ts.tv_sec + ts.tv_nsec;
}
*/


int enableServer(int port, struct NetworkCallbacks1 cbs) {

	int status = ensureEnetInitialized(); if (status < 0) return -1;

	ENetAddress address;

	address.host = ENET_HOST_ANY;
	address.port = port;

	ENetHost *host = enet_host_create(&address, MAX_CONNECTIONS, 2, 0, 0);

	if (host == NULL) {
		fprintf(stderr, "enet_host_create: failed\n");
		return -1;
	}

	server.host = host;
	server.callbacks = cbs;
	server.enabled = true;

	return 0;

}

int connectToServer(const char * hostname, int port, struct NetworkCallbacks2 cbs) {

	if (client.enabled) {
		fprintf(stderr, "connectToServer: clientEnabled already\n");
		return -1;
	}
	
	int status;

	status = ensureEnetInitialized(); if (status < 0) return -1;

	ENetAddress address;

	status = enet_address_set_host(&address, hostname);
	if (status < 0) {
		fprintf(stderr, "enet_address_set_host: failed to resolve hostname\n");
		return -1;
	}

	address.port = port;

	ENetHost *host = enet_host_create(NULL, 1, 2, 0, 0);

	if (host == NULL) {
		fprintf(stderr, "enet_host_create: failed\n");
		return -1;
	}

	ENetPeer *peer = enet_host_connect(host, &address, 1, 0);

	if (peer == NULL) {
		fprintf(stderr, "enet_host_connect: failed\n");
		return -1;
	}

	client.host = host;
	client.peer = peer;
	client.enabled = true;
	client.callbacks = cbs;

	return 0;
}

void disableServer() {
	if (server.enabled == false) return;

	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if(conns[i].active && conns[i].peer){
			enet_peer_disconnect(conns[i].peer, 0);
			enet_host_flush(server.host);
		}
		conns[i].active = 0;
		conns[i].peer = NULL;
	}

	enet_host_destroy(server.host);

	server.host = NULL;
	server.enabled = false;
}

void disconnectFromServer() {
	if (client.enabled == false) return;

	enet_peer_disconnect(client.peer, 0);

	enet_host_flush(client.host);

	enet_host_destroy(client.host);

	client.peer = NULL;
	client.host = NULL;
	client.enabled = false;
}


int sendMessage(unsigned char * data, int datasize) {
	if (client.enabled == false) return -1;

	ENetPacket * packet = enet_packet_create(data, datasize, ENET_PACKET_FLAG_RELIABLE);

	int status = enet_peer_send(client.peer, 0, packet);

	if (status < 0) {
		fprintf(stderr, "(%d) enet_peer_send: failed\n", __LINE__);
		return -1;
	}

	enet_host_flush(client.host);

	return 0;
}

int sendPacketTo(int connId, int channel, unsigned char * data, int datasize) {
	if (server.enabled == false) return -1;

	ENetPacket * packet = enet_packet_create(data, datasize, ENET_PACKET_FLAG_RELIABLE);

	ENetPeer * peer = conns[connId].peer;

	if (peer == NULL) {
		fprintf(stderr, "(%d) sendPacketTo: bad connId\n", __LINE__);
		return -1;
	}

	int status = enet_peer_send(peer, channel, packet);

	if (status < 0) {
		fprintf(stderr, "(%d) enet_peer_send: failed\n", __LINE__);
		return -1;
	}

	enet_host_flush(server.host);

	return 0;
}

int sendMessageTo(int connId, unsigned char * data, int datasize) {
	return sendPacketTo(connId, 0, data, datasize);
}

int sendChunkTo(int connId, unsigned char * data, int datasize) {
	return sendPacketTo(connId, 1, data, datasize);
}


void closeConnection(int connId) {
	ENetPeer *peer = conns[connId].peer;

	if (peer == NULL) {
		fprintf(stderr, "(%d) closeConnection: bad connId\n", __LINE__);
		return;
	}

	enet_peer_disconnect(peer, 0);
	enet_host_flush(server.host);

	conns[connId].active = 0;
	conns[connId].peer = NULL;
	conns[connId].identifier[0] = 0;
}


static void pollClient(int timeout){

	ENetEvent event;

	for (;;) {
		int status = enet_host_service(client.host, &event, timeout);

		if(status == 0) return;

		if(event.type == ENET_EVENT_TYPE_CONNECT) {
			// connection to server established
			client.callbacks.csc();
			client.connected = true;
			continue;
		}

		if(event.type == ENET_EVENT_TYPE_RECEIVE) {
			// message available on some channel

			int channel = event.channelID;

			if(channel == 0){
				// normal message
				client.callbacks.nmc(0, event.packet->data, event.packet->dataLength);
				enet_packet_destroy(event.packet);
			}
			else if (channel == 1){
				// a chunk
				client.callbacks.nchc(0, event.packet->data, event.packet->dataLength);
				enet_packet_destroy(event.packet);
			}
			else{
				fprintf(stderr, "pollClient: received packet on bad channel %d\n", channel);
				enet_packet_destroy(event.packet);
			}
			continue;
		}

		if(event.type == ENET_EVENT_TYPE_DISCONNECT){
			if(client.connected){
				// server connection went away
				client.callbacks.ccc();
			}
			else{
				// connection attempt failed
				client.callbacks.cfc(-1);
			}
			enet_host_destroy(client.host);
			client.host = NULL;
			client.enabled = false;
			client.connected = false;
			return;
		}

		fprintf(stderr, "pollClient: bad event type %d\n", event.type);
	}
}



static void pollServer(int maxtimeout) {
	ENetEvent event;

	int timeout = maxtimeout;
	// check for ongoing large message streaming in which case you want less timeout

	for (;;) {
		int status = enet_host_service(server.host, &event, timeout);

		if(status == 0) return;

		if(event.type == ENET_EVENT_TYPE_CONNECT) {
			intptr_t connId = getUnusedConnId();
			if (connId < 0) {
				fprintf(stderr, "pollServer: not enough room for new connection\n");
				enet_peer_reset(event.peer);
				continue;
			}

			event.peer->data = (void*)connId;
			conns[connId].peer= event.peer;

			// generate identifier
			char identifier[64];
			enet_address_get_host_ip(&event.peer->address, identifier, 64);
			strcat(identifier, " ");
			char temp[64];
			sprintf(temp, "%u", event.peer->address.port);
			strcat(identifier, temp);
			server.callbacks.ncc(connId, identifier);
			continue;
		}

		if(event.type == ENET_EVENT_TYPE_RECEIVE) {
			// new message (large message not allowed here)
			intptr_t connId = (intptr_t)event.peer->data;
			server.callbacks.nmc(connId, event.packet->data, event.packet->dataLength);
			enet_packet_destroy(event.packet);
			continue;
		}

		if(event.type == ENET_EVENT_TYPE_DISCONNECT){
			intptr_t connId = (intptr_t)event.peer->data;
			server.callbacks.dc(connId);
			conns[connId].active = 0;
			conns[connId].peer = NULL;
			continue;
		}

		fprintf(stderr, "pollServer: bad event type %d\n", event.type);
	}
}

// check the hosts and see if any activity needs to be serviced with callbacks
void pollNetwork() {

	if (client.enabled) pollClient(0);
	if (server.enabled) pollServer(0);

}

void pollNetworkDedicated() {

	if (client.enabled) pollClient(1000);
	if (server.enabled) pollServer(1000);

}
