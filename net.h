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
int waitForInbox(struct Mailbox *m);
int sendMessageToClient(struct Mailbox *m);
struct Mailbox * mallocMailbox();
void cloneMailbox(struct Mailbox *dst, struct Mailbox *src);

void printMailbox(struct Mailbox *m);

struct sockaddr_in localhostPort(int port);

int compareAddress(struct sockaddr_storage *ssA, struct sockaddr_storage *ssB);

void scribble8(struct Mailbox *m, unsigned char c);
void scribbleChar(struct Mailbox *m, char c);
void scribbleString(struct Mailbox *m, const char *str);
void scribble16(struct Mailbox *m, unsigned int n);
void scribble32(struct Mailbox *m, unsigned int n);
void scribble64(struct Mailbox *m, unsigned long long n);
void scribbleFloat(struct Mailbox *m, float x);
void scribbleDouble(struct Mailbox *m, double x);