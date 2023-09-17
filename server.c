/* the server runs in a dedicated thread waiting for messages.
It acts as a second "user" module which can access many things.
When managing the protocol it should need to synchronize with
the main thread. When RPC calls come in they are distributed
to all the players. Other protocol actions may cause messages
to be distributed to other players to keep them informed. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <threads.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <net.h>

#include <clocks.h>

static struct Mailbox *outboxes[1024];
static struct Mailbox **outboxes_ptr = outboxes;
static struct Mailbox **outboxes_end = outboxes + 1024;

static struct Mailbox ** findOutbox(struct sockaddr_storage *addr){
	for(struct Mailbox **pptr = outboxes; pptr < outboxes_ptr; pptr++){
		if(compareAddress(addr, &(*pptr)->addr) == 0) return pptr;
	}
	return NULL;
}

static void listOutboxes(){
	printf("** listing outboxes **\n");
	
	for(struct Mailbox **pptr = outboxes; pptr < outboxes_ptr; pptr++){
		printMailbox(*pptr);
	}
}

void printMessage(unsigned char buf[], int n){
	for(int i = 0; i < n; i++){
		unsigned char byte = buf[i];
		if(isprint(byte) && byte != ' '){
			printf("%c ", byte);
		}
		else{
			printf("#%u ", byte);
		}
	}

	puts("");
}

void addOutbox(struct Mailbox *m){
	if(outboxes_ptr == outboxes_end){
		fprintf(stderr, "addOutbox: too many outboxes\n");
		exit(1);
	}

	*outboxes_ptr++ = m;
}

void endCommunication(struct Mailbox *outbox){
	for(struct Mailbox **mpp = outboxes; mpp < outboxes_ptr; mpp++){
		if(*mpp != outbox) continue;
		printf("deleting outbox\n");
		free(outbox);
		if(mpp < outboxes_end)
			*mpp = *--outboxes_ptr;
		else
			--outboxes_ptr;
		return;
	}
}

void expireOutboxes(double currentTime){
	for(struct Mailbox **mpp = outboxes_ptr - 1; mpp >= outboxes; mpp--){
		if((*mpp)->deathtime <= currentTime) endCommunication(*mpp);
	}
}

/*
void speakProtocol(struct Mailbox *inbox, struct Mailbox *outbox, double currentTime){
	int ok;
	int error;

	outbox->datasize = 0;

	switch(outbox->state){
		case PS_START: // They said KNOCK KNOCK
			scribbleString(outbox, "WHO'S THERE\n");
			sendToClient(outbox);
			outbox->state = PS_WHO_IS_IT;
			return;

		case PS_WHO_IS_IT:
			if(!messageTitleEquals(inbox->data, "CREDENTIALS")){
				endCommunication(outbox);
				return;
			}

			// check credentials

			scribbleString(outbox, "SECRET WORD\n");
			sendToClient(outbox);
			outbox->state = PS_SECRET_WORD;
			return;

		case PS_SECRET_WORD:
			if(!messageTitleEquals(inbox->data, "WHISPER")){
				endCommunication(outbox);
				return;
			}

			// check password

			scribbleString(outbox, "SYNC CLOCKS\n");
			sendToClient(outbox);
			outbox->state = PS_SYNC_CLOCKS;
			return;

		case PS_SYNC_CLOCKS:
			if(messageTitleEquals(inbox->data, "PING")){
				struct PingMessage ping = parsePingMessage(inbox->data, &error);
				if(error){
					endCommunication(outbox);
					return;
				}

				scribbleString(outbox, "PONG\n");
				scribble8(outbox, ping.sequence); scribbleChar(outbox, '\n');
				scribbleDouble(outbox, ping.sendTime); scribbleChar(outbox, '\n');
				scribbleDouble(outbox, currentTime); scribbleChar(outbox, '\n');
				sendToClient(outbox);
				return;
			}
			else if(messageTitleEquals(inbox->data, "CONFIRMED")){
				//begin streaming process
				//stream what? entire game state split into identified chunks
				//how fast to stream it, depends
				//what code where is streaming it
				//  the streamer
				outbox->state = PS_STREAMING;
				return;
			}
			else {
				endCommunication(outbox);
				return;
			}

		case PS_STREAMING:
			// in the streaming state they may request missed chunks
			// GET CHUNK [n]
			// or confirm they got all chunks and the game can unpause
			// COMPLETE
			if(messageTitleEquals(inbox->data, "COMPLETE")){
				return;
			}
			else{
			}
		case PS_LINKALIVE:
		case PS_LINKDEAD:
		default:
			fprintf(stderr, "speakProtocol: bad proto state %d\n", outbox->state);
			endCommunication(outbox);
	}
}
*/

void speakProtocol(struct Mailbox *inbox, struct Mailbox *outbox, double currentTime){
}


int unspawnServerInboxThread(struct Mailbox *m){
	m->stopflag = true;

	struct sockaddr_in addr = localhostPort(m->port);
	socklen_t addrlen = sizeof addr;

	// send dummy msg to wake up thread if necessary
	int num = sendto(m->socket, NULL, 0, 0, (struct sockaddr*)&addr, addrlen);
	if(num < 0){
		fprintf(stderr, "(unspawnServerInboxThread) sendto: %s\n", strerror(errno));
		return -1;
	}

	fprintf(stderr, "joining inbox thread... ");

	int e = thrd_join(m->thread, NULL);

	if(e != thrd_success){
		fprintf(stderr, "thrd_join: error %d\n", e);
		return -1;
	}

	fprintf(stderr, "joined\n");

	return 0;
}


static int serverInboxThread(void *u){

	struct Mailbox *m = u;

	for (;;) {
		if (m->stopflag) break;

		waitForInbox(m);

		if (m->stopflag) break;

		// m->data contains message of size m->datasize
		m->dataflag = true;

		double currentTime = chronf();

		// process the message as much as possible from this thread
		// HERE
		struct Mailbox **p = findOutbox(&m->addr);
		struct Mailbox *outbox;

		if(p == NULL){
			printf("unknown caller: ");
			printMessage(m->data, m->datasize);
			// caller not recognized, check the message to see if they should be ignored
			if(memcmp(m->data, "KNOCK\nKNOCK\n", 12) != 0) continue;
			if(outboxes_ptr == outboxes_end){
				printf("too many outboxes\n");
				dumbSendTo(m->socket, &m->addr, m->addrlen, "BUSY\n");
				continue;
			}
			outbox = mallocMailbox();
			cloneMailbox(outbox, m);
			*outboxes_ptr++ = outbox;
			outbox->deathtime = currentTime + 1;
			outbox->state = PS_WHO_IS_IT;
			outbox->counter = 1;
		}
		else{
			printf("previous caller\n");
			outbox = *p;
			outbox->counter++;
			if(outbox->counter > 255) outbox->counter = 0;
		}

		// caller has overstayed their welcome
		if(outbox->deathtime <= currentTime){
			printf("caller has expired\n");
			endCommunication(outbox);
			continue;
		}
		else{
			printf("extending caller life\n");
			// they get a few more seconds of life
			outbox->deathtime = currentTime + 10;
		}

		printf("server inbox got mail: ");
		printMessage(m->data, m->datasize);

		printf("protocol\n");
		speakProtocol(m, outbox, currentTime);

		printf("expire\n");
		expireOutboxes(currentTime);

		printf("list outboxes\n");
		listOutboxes();
		printf("loop\n");
	}
	
	return 0;
}

int spawnServerInboxThread(struct Mailbox *m){
	m->stopflag = false;
	int e = thrd_create(&m->thread, serverInboxThread, m); if(e != thrd_success){ return -1; }
	return 0;
}

int spawnServer(int port, struct Mailbox *m){
	puts("spawnServer");
	int status;
	status = UDPServer(port, m);        if (status < 0) return -1;
	status = spawnServerInboxThread(m); if (status < 0) return -1;
	return 0;
}

int unspawnServer(struct Mailbox *m){
	puts("unspawnServer");
	int status;
	status = unspawnServerInboxThread(m);
	closeMailbox(m);

	puts("reclaim outboxes");
	for(struct Mailbox **ppm = outboxes; ppm < outboxes_ptr; ppm++){
		free(*ppm);
	}
	outboxes_ptr = outboxes;

	return status;
}
