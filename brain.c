/*
	brain

	where a lot of stuff that shouldn't have been in renderer went

*/

#include <stdio.h>
#include <stdbool.h>

#include <engine.h>
#include <linear.h>
#include <sound.h>
#include <physics.h>
#include <chunk.h>
#include <megaman.h>
#include <network.h>
#include <messages.h>
#include <clocks.h>
#include <misc.h>

struct Megaman megaman;

static int stillCalling = 0;
static int callingTimer = 240;
static bool clientEn = false;

void initializeEverything(){
	finishChunkLoading();
}

void newConnectionCb(int connId, const char * identifier){
	printf("New Connection connId=%d identifier=%s\n", connId, identifier);
	playSound(SND_DSRADIO);
}

void newMessageCb(int connId, unsigned char * data, int datasize){
	printf("New Message from connId=%d: ", connId);
	printData(data, datasize);

	playSound(SND_MURMUR);

	struct Ping ping;
	int e = parsePing(data, datasize, &ping);
	if(e < 0){ printf("failed to parse PING\n"); return; }

	printf("Ping %d %lf\n", ping.sequence, ping.time);

	double t = chronf();
	struct Pong pong = {ping.sequence, ping.time, t};
	unsigned char buf[256];
	int size = unparsePong(&pong, buf, 256);
	if(size < 0){ printf("failed to unparse PONG\n"); return; }
	e = sendMessageTo(connId, buf, size);
	if(e < 0){ printf("failed to send PONG\n"); return; }
}

void disconnectionCb(int connId){
	printf("connId=%d disconnected\n", connId);
	playSound(SND_DSRADIO);
}

void newMessageCb2(int connId, unsigned char * data, int datasize){
	printf("Client got New Message connId=%d: ", connId);
	printData(data, datasize);

	playSound(SND_MURMUR);

	struct Pong pong;
	int e = parsePong(data, datasize, &pong);
	if(e < 0){ printf("failed to parse PONG\n"); return; }

	double now = chronf();
	printf("Pong %d %lf %lf now=%lf\n", pong.sequence, pong.time1, pong.time2, now);
	printf("time1 = %lf\n", pong.time1);
	printf("time2 = %lf\n", now);
	printf("time2 - time1 = %lf\n", now - pong.time1);
}

void newChunkCb(unsigned char * data, int datasize){
	printf("Client got New Chunk: ");
	printData(data, datasize);
	playSound(SND_MURMUR);
}

void connectionSucceededCb(void){
	stillCalling = 0;
	callingTimer = 240;
	engine.networkStatus = CLIENT;
	printf("connection succeeded\n");
	playSound(SND_SUCCESS);
	stopSound(SND_CALLING);
}

void connectionFailedCb(int error){
	stillCalling = 0;
	callingTimer = 240;
	engine.networkStatus = OFFLINE;
	printf("connection failed\n");
	playSound(SND_WRONG);
	stopSound(SND_CALLING);
}

void connectionClosedCb(void){
	stillCalling = 0;
	clientEn = false;
	engine.networkStatus = OFFLINE;
	printf("connection closed\n");
	playSound(SND_CLOSED);
}



int enableServer(){
	struct NetworkCallbacks1 serverCallbacks = {
		newConnectionCb,
		newMessageCb,
		disconnectionCb
	};

	puts("Host Game ...");
	int status = netEnableServer(engine.serverPort, serverCallbacks);
	if(status < 0)  return -1;

	engine.networkStatus = SERVER;
	puts("... Server Online");
	return 0;
}


void pressH(){

	switch (engine.networkStatus) {
	case SERVER:
		netDisableServer();
		fprintf(stderr, "Server terminated\n");
		playSound(SND_CLOSED);
		engine.networkStatus = OFFLINE;
		break;
	case CONNECTING:
		fprintf(stderr, "Connection in progress\n");
		playSound(SND_WRONG);
		break;
	case CLIENT:
		fprintf(stderr, "You're in the middle of a multiplayer game already\n");
		playSound(SND_WRONG);
		break;
	case OFFLINE:
		int status = enableServer();
		if (status < 0) {
			playSound(SND_WRONG);
		} else {
			playSound(SND_GRANTED);
		}
		break;
	}

}

void pressN(){
	pollNetwork();
}

void pressC(){

	switch (engine.networkStatus) {
	case SERVER:
		fprintf(stderr, "You're hosting a game. Shutdown the server first\n");
		playSound(SND_WRONG);
		break;
	case CLIENT:
	case CONNECTING:
		disconnectFromServer();
		playSound(SND_CLOSED);
		stopSound(SND_CALLING);
		engine.networkStatus = OFFLINE;
		callingTimer = 240;
		stillCalling = 0;
		clientEn = false;
		break;
	case OFFLINE:
		struct NetworkCallbacks2 clientCallbacks = {
			.csc = connectionSucceededCb,
			.cfc = connectionFailedCb,
			.ccc = connectionClosedCb,
			.nmc = newMessageCb2,
			.nchc = newChunkCb,
		};
		int status = connectToServer(engine.serverHostname, engine.serverPort, clientCallbacks);
		if(status < 0) return;
		clientEn = true;
		playSound(SND_CALLING);
		stillCalling = 1;
		engine.networkStatus = CONNECTING;
		break;
	}
}

void pressL(){
	unsigned char msg[] = "HELLO WORLD\n";
	int status = sendMessageTo(0, msg, sizeof msg);
	if(status < 0){
		printf("sendMessage failed!\n");
	}
}

void pressG(){
	unsigned char msg[] = "HELLO WORLD\n";
	int status = sendMessage(msg, sizeof msg);
	if(status < 0){
		printf("sendMessage failed!\n");
	}
}

void pressP(){
	double t = chronf();
	struct Ping ping = {0, t};
	unsigned char buf[1024];
	int size = unparsePing(&ping, buf, 1024);
	int e = sendMessage(buf, size);
	if(e<0){ printf("ping failed to send\n"); return; }

	double timeout = t + 0.250;

	while(chronf() < timeout){
		pollNetwork();
	}
}

// do one update to the world
void think(){

	if(stillCalling){
		callingTimer--;
		if(callingTimer == 0){
			callingTimer = 350;
			playSound(SND_CALLING);
		}
	}

	physics();

}


void shutdownEverything(){ // (shutdown everything except renderer)
	disconnectFromServer();
	netDisableServer();
}
