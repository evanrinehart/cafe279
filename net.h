#define MAILBOX_SIZE 1024

struct Mailbox {
	int socket;                   // handle to the UDP socket
	int port;                     // port socket was binded to
	struct sockaddr_storage addr; // address storage big enough for any protocol
	socklen_t addrlen;            // size of address storage
	thrd_t thread;
	int error_number;
	bool stopflag;
	bool dataflag;  
	int datasize;
	unsigned char data[MAILBOX_SIZE];
};

int UDPServer(int port, struct Mailbox *m);
void closeMailbox(struct Mailbox *m);
void voidMailbox(struct Mailbox *m);
int waitForInbox(struct Mailbox *m);
int sendMessageToClient(struct Mailbox *m);
struct Mailbox * mallocMailbox();

int spawnServerInboxThread(struct Mailbox *m);
int unspawnServerInboxThread(struct Mailbox *m);
