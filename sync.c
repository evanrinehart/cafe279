/*
	sync.c

	The purpose of this algorithm is

	* perform several measurements of round trip time to the server
	* then produce an estimated clock offset between local and server time
	* then update engine.timeOffset

	From this point on our local notion of serverTime will be close to accurate.

	The process takes time so has the form of a resumable procedure.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sync.h>

#include <clocks.h>
#include <messages.h>
#include <network.h>

enum SyncStatus syncStatus = SYNC_INACTIVE;

static int samplesLeft = 0;
static int sequenceCounter = 0;
static int cooldown = 0;

#define NUM_SAMPLES 13
#define COOLDOWN_TIME 11

static struct Sample samples[NUM_SAMPLES];

void printSamples(){
	for(int i = 0; i < NUM_SAMPLES; i++){
		if(samples[i].blank) continue;

		printf("sample %d: %lf %lf %lf %lf\n",
			i,
			samples[i].time1,
			samples[i].serverTime,
			samples[i].time2,
			samples[i].rtt
		);
			
	}
}

static int compareSamples(const struct Sample *s1, const struct Sample *s2){
	if(s1->rtt < s2->rtt) {
		return -1;
	}
	else if(s1->rtt > s2->rtt) {
		return 1;
	}
	else {
		return 0;
	}
}

typedef int (*cmpFunc)(const void * v1, const void * v2);

static void sortSamples(){
	qsort(samples, NUM_SAMPLES, sizeof (struct Sample), (cmpFunc)compareSamples);
}

int sendPing(int sequence, double time){
	struct Ping ping = {sequence, time};

	static unsigned char pingBuf[256];

	struct Slice bufslice = {pingBuf, 256};

	int size = unparsePing(&ping, &bufslice);
	if(size < 0){ fprintf(stderr, "failed to encode ping message\n"); return -1; }

	int status = sendMessage(pingBuf, size);
	if(status < 0){ fprintf(stderr, "sendPing failed\n"); return -1; }

	return 0;
}

void syncPoll(){
	if(syncStatus != SYNC_WORKING) return;

	if(cooldown > 0){
		cooldown--;
		return;
	}
	else{
		double now = chronf();
		int i = sequenceCounter;
		int status = sendPing(i, now); if (status < 0) return;
		sequenceCounter++;
		cooldown = COOLDOWN_TIME;
		printf("sendPing %d at %lf\n", i, now);
	}
}

void syncPong(int sequence, double time1, double serverTime, double time2){
	if(syncStatus != SYNC_WORKING) return;
	if(sequence < 0 || sequence > NUM_SAMPLES - 1) return;
	printf("got pong %d\n", sequence);
	samples[sequence].time1 = time1;
	samples[sequence].serverTime = serverTime;
	samples[sequence].time2 = time2;
	samples[sequence].rtt = time2 - time1;
	samples[sequence].blank = false;
	samplesLeft--;
	printf("samples left = %d\n", samplesLeft);
	if(samplesLeft == 0) syncStatus = SYNC_READY;
}

void syncBegin(){
	syncStatus = SYNC_WORKING;
	cooldown = 0;
	samplesLeft = NUM_SAMPLES;
	sequenceCounter = 0;

	for(int i = 0; i < NUM_SAMPLES; i++){
		samples[i].blank = true;
	}
}

double syncEnd(){
	if(syncStatus != SYNC_READY){
		fprintf(stderr, "syncEnd called at a bad time\n");
		return 0;
	}

	sortSamples();

	printf("sorting samples\n\n");

	printSamples();

	struct Sample *best = &samples[0]; // fastest sample has least margin of error

	double oneWayTime = best->rtt / 2;

	double answer = best->serverTime - (best->time1 + oneWayTime);

	printf("answer = %lf\n", answer);

	syncStatus = SYNC_INACTIVE;

	return answer;
}
