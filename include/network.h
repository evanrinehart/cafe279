typedef void (*newConnectionCallback)(int connId, const char * identifier);
typedef void (*newMessageCallback)(int connId, unsigned char * data, int datasize);
typedef void (*disconnectionCallback)(int connId);

typedef void (*connectionSucceededCallback)(void);
typedef void (*connectionFailedCallback)(int error);
typedef void (*connectionClosedCallback)(void);
typedef void (*newChunkCallback)(unsigned char * data, int datasize);


// server mode
struct NetworkCallbacks1 {
	newConnectionCallback ncc;
	newMessageCallback nmc;
	disconnectionCallback dc;
};

// client mode
struct NetworkCallbacks2 {
	connectionSucceededCallback csc;
	connectionFailedCallback cfc;
	connectionClosedCallback ccc;
	newMessageCallback nmc;
	newChunkCallback nchc;
};

int netEnableServer(int port, struct NetworkCallbacks1 cbs);
void netDisableServer();

int connectToServer(const char * hostname, int port, struct NetworkCallbacks2 cbs);
int sendMessage(unsigned char * data, int datasize);
void disconnectFromServer();

int sendMessageTo(int connId, unsigned char * data, int datasize);
int sendChunkTo(int connId, unsigned char * data, int datasize);
void closeConnection(int connId);

void pollNetwork();
void pollNetworkTimeout(int ms);

double getPingTime();
double getPingTimeTo(int connId);
