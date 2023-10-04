/*
	brain

	where a lot of stuff that shouldn't have been in renderer went

*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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

int chosen_block = 255;

void clickTile(int i, int j, int ctrl){
	int *b = &chunk.block[i][j];

	if(ctrl){
		*b = 0;
	}
	else{
		*b = chosen_block;
	}
}

void pressNumber(int d) {
	int a = chosen_block / 100;
	int b = (chosen_block % 100) / 10;
	int c = chosen_block % 10;

	int x = b;
	int y = c;
	int z = d;

	chosen_block = x * 100 + y * 10 + z;

	printf("a b c = %d %d %d\n", a, b, c);
	printf("x y z = %d %d %d\n", a, b, c);
	printf("chosen block = %d\n", chosen_block);
}

void rightClickTile(int i, int j){
	int *b = &chunk.block[i][j];
	*b = 0;
}

void pressZ(){
	looseItems_ptr = looseItems;
}

#define world2raw(X) (((X) + 2048) * 256)
#define raw2world(I) (((I) / 256.0) - 2048.0)
#define double2raw(X) ((X) * 256.0)
#define raw2double(I) ((I) / 256.0)

void pressI(){
	double x = 16*4 + randi(-30,30);
	double y = 16*4 + randi(-30,30);

	//double x= 16*3;
	//ldouble y= 16*4;

	struct LooseItem item;
	item.i = world2raw(x);
	item.j = world2raw(y);

	item.angle = 0;
	item.spin  = 0;

	item.di = randi(-256,256);
	item.dj = randi(-256,256);
	//item.di = 50;
	//item.dj = 100;

/*
	item.i = 542944 - 239;
	item.j = 531632 - 93;
	item.di = -239;
	item.dj = -93;
*/

//printf("%d %d %d %d\n", item.i, item.j, item.di, item.dj);

/*
	chunk.block[130][129] = 192;
	chunk.block[131][129] = 193;
	chunk.block[132][129] = 194;
	chunk.block[133][129] = 195;
	chunk.block[134][129] = 196;


	chunk.block[130][129] = 224;
	chunk.block[131][129] = 225;
	chunk.block[132][129] = 226;
	chunk.block[133][129] = 227;
	chunk.block[134][129] = 228;


	chunk.block[130][129] = 240;
	chunk.block[131][129] = 241;
	chunk.block[132][129] = 242;
	chunk.block[133][129] = 243;
	chunk.block[134][129] = 244;

	chunk.block[130][129] = 245;
	chunk.block[131][129] = 246;
	chunk.block[132][129] = 247;
	chunk.block[133][129] = 248;
	chunk.block[134][129] = 249;
*/

	addLooseItem(&item);
	//printf("ptr - base = %ld\n", looseItems_ptr - looseItems);
}

#define PI M_PI

vec2 vec2FromNormal(int n) {
	double angle = 2.0 * PI * n / 4096.0 + PI / 2;
	return cis(angle);
}

int ccount = 0;

/* particle-type collision algorithm

- a particle has spatial extent and "sensors" on 4 sides.
- at most 2 sensors are used to determine imminent collision
- first, if either sensor determines that the particle is partially inside a solid
	the particle is minimum moved out along that axis
- second, if velocity in either direction would cross the surface, a collision is reported

- the X sensor and Y sensor data can be collected before acting on the particle or issuing a collision

*/

struct Particle {
	int x, y;
	int dx, dy;
};

/* moving particle p is depenetrated from any solid it is moving into then
it is moved as far as it can go before hits a surface. In which case normal is
set to the collision normal and 1 is returned.  If no collision occurs 0 is
returned. */
int collideParticle(struct Particle * p, vec2 * normal) {
	int standoff = 1;
	int n;

	int flag = 0;

	if(p->dy < 0){
		int ping = probeDown(p->x, p->y - standoff, &n);
		if(ping >= 0 && -p->dy <= ping){ // any surface is beyond the range of desired jump
			p->y += p->dy; // move normally
		}
		else{
			p->y -= ping; // moves p to collision point, which might be backwards if depenetrating
			*normal = vec2FromNormal(n);
			flag = 1;
		}
	}
	else if(p->dy > 0){
		int ping = probeUp(p->x, p->y + standoff, &n);
		if(ping >= 0 && p->dy <= ping){
			p->y += p->dy;
		}
		else{
			p->y += ping;
			*normal = vec2FromNormal(n);
			flag = 1;
		}
	}

	if(p->dx < 0){
		int ping = probeLeft(p->x - standoff, p->y, &n);
		if(ping >= 0 && -p->dx <= ping){
			p->x += p->dx;
		}
		else{
			p->x -= ping;
			*normal = vec2FromNormal(n);
			flag = 1;
		}
	}
	else if(p->dx > 0){
		int ping = probeRight(p->x + standoff, p->y, &n);
		if(ping >= 0 && p->dx <= ping){
			p->x += p->dx;
		}
		else{
			p->x += ping;
			*normal = vec2FromNormal(n);
			flag = 1;
		}
	}

	return flag;

}



void updateLoose() {

	for(struct LooseItem * item = looseItems; item < looseItems_ptr; item++){

		item->angle += item->spin;

		int hitx;
		int hity;
		vec2 hitn;

		struct Particle p = {item->i, item->j, item->di, item->dj};
		int hit = collideParticle(&p, &hitn);
		item->i = p.x;
		item->j = p.y;

		if (hit) {
			printf("%f hit %d %d (%lf, %lf)\n", chronf(), hitx, hity, hitn.x, hitn.y);
			//printf("before v = %d %d\n", item->di, item->dj);
			vec2 v = vec2(item->di, item->dj);
			vec2 vprime = reflection(v, rotate(hitn,PI/2));
			item->di = vprime.x;
			item->dj = vprime.y;
			//printf("after v = %d %d\n", item->di, item->dj);
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

void spawnItem(double x, double y){
	struct LooseItem item;

	item.i = world2raw(x + 8);
	item.j = world2raw(y + 8);
	item.di = 128;
	item.dj = 128;
	item.angle = 0;
	item.spin = 0;

	addLooseItem(&item);
}
