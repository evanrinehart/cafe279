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

		// process the message as much as possible from this thread
		// HERE
		struct Mailbox **p = findOutbox(&m->addr);
		struct Mailbox *outbox;

		if(p == NULL){
			outbox = mallocMailbox();
			cloneMailbox(outbox, m);
			*outboxes_ptr++ = outbox;
		}
		else{
			outbox = *p;
		}

		double t = chronf();
		outbox->datasize = 0;
		scribbleString(outbox, "PONG\n");
		scribbleDouble(outbox, t);
		scribbleString(outbox, "\n");
		printf("sending: "); printMessage(outbox->data, outbox->datasize);
		int status = sendMessageToClient(outbox);

		if(status < 0){
			printf("send failed, trashing outbox\n");
			*p = *--outboxes_ptr;
		}

		printf("server inbox got mail: ");
		printMessage(m->data, m->datasize);

		listOutboxes();
		// data is consumed now

		m->datasize = 0;
		m->dataflag = false;
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
