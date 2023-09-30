/*
	brain

	where a lot of stuff that shouldn't have been in renderer went

*/

#include <stdio.h>
#include <stdbool.h>

#include <math.h>

#include <engine.h>
#include <linear.h>
#include <sound.h>
#include <physics.h>
#include <chunk.h>
#include <megaman.h>
#include <doodad.h>
#include <network.h>
#include <messages.h>
#include <clocks.h>
#include <misc.h>

#include <items.h>

#include <sync.h>

struct Megaman megaman;

static int stillCalling = 0;
static int callingTimer = 240;
static bool clientEn = false;

void initializeEverything(){
	finishChunkLoading();
}


void syncComplete(double offset){
	printf("sync done offset = %lf\n", offset);
	engine.timeOffset = offset;
	// continue joining game
}

void syncFailed(){
	printf("Sync failed. Disconnecting\n");
	disconnectFromServer();
	engine.networkStatus = OFFLINE;
	playSound(SND_CLOSED);
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
	struct Slice dataslice = {data, datasize};
	int e = parsePing(&dataslice, &ping);
	if(e < 0){ printf("failed to parse PING\n"); return; }

	printf("Ping %d %lf\n", ping.sequence, ping.time);

	double t = chronf();
	struct Pong pong = {ping.sequence, ping.time, t};
	unsigned char buf[256];
	struct Slice bufslice = {buf, 256};
	int size = unparsePong(&pong, &bufslice);
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
	struct Slice dataslice = {data, datasize};
	int e = parsePong(&dataslice, &pong);
	if(e < 0){ printf("failed to parse PONG\n"); return; }

	double now = chronf();

	syncPong(pong.sequence, pong.time1, pong.serverTime, now);
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

	struct SyncCallbacks cbs = {syncComplete, syncFailed};

	syncBegin(cbs);
}

void connectionFailedCb(int error){
	stillCalling = 0;
	callingTimer = 240;
	engine.networkStatus = OFFLINE;
	printf("connection failed (%d)\n", error);
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
	case OFFLINE:
	{
		int status = enableServer();
		if (status < 0) {
			playSound(SND_WRONG);
		} else {
			playSound(SND_GRANTED);
		}
		break;
	}
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
	}

}

void pressN(){
	pollNetwork();
}

void pressC(){

	switch (engine.networkStatus) {
	case OFFLINE:
	{
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
	struct Slice slice = {buf, 1024};
	int size = unparsePing(&ping, &slice);
	int e = sendMessage(buf, size);
	if(e<0){ printf("ping failed to send\n"); return; }
}

void pressR(){
	addObject();
	showRooms();

	for(struct Doodad *d = doodads; d < doodads_ptr; d++) printDoodad(d);
}

void pressLeft(){
}

void pressRight(){
}

double xvel = 0;
double xacc = 0;
double xvelR = 0;
double xvelL = 0;

void pressWASD(char wasd, int down){
	printf("%c %s\n", wasd, down ? "down" : "up");
	if(wasd=='a') xvelL = down;
	if(wasd=='d') xvelR = down;
	xacc = (xvelR - xvelL) / 4;
	if(xacc == 0) {
		xvel = 0;
	}
}

void clickTile(int i, int j, int ctrl){
	int b = chunk.block[i][j];

	if(ctrl){
		chunk.block[i][j] = 0;
	}
	else{
		chunk.block[i][j] = 255;
	}
}

void pressI(){
	int i = 2048 + 16*4 + randi(-30, 30);
	int j = 2048 + 16*4 + randi(-30, 30);
	struct LooseItem item;
	item.i = i;
	item.j = j;
	item.code = 0;
	item.rotation = 0;

	item.di = randi(-1,1);
	item.dj = randi(-1,1);
	if(item.di==0 && item.dj==0) item.di = 1;
	item.timer = 3;

	printf("adding item i=%d j=%d\n", i, j);
	addLooseItem(&item);
	printf("ptr - base = %ld\n", looseItems_ptr - looseItems);
}

void updateLoose() {
	for(struct LooseItem * item = looseItems; item < looseItems_ptr; item++){

		if(item->timer == 0){
			item->i += item->di;
			item->j += item->dj;
			item->timer = 3;
		}
		else{
			item->timer--;
		}

	}

}

void inputCharacter(int c){
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

	syncPoll();

	physics();

	int MAX = 4;

	xvel += xacc;
	if(xvel >  MAX) xvel =  MAX;
	if(xvel < -MAX) xvel = -MAX;
	megaman.x += xvel;

	//printf("xvel = %lf\n", xvel);

	updateLoose();

}

void chill() {
	if(engine.networkStatus == OFFLINE) millisleep(1);
	else pollNetworkTimeout(3);
}

void shutdownEverything(){ // (shutdown everything except renderer)
	disconnectFromServer();
	netDisableServer();
}
