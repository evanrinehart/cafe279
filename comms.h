typedef struct ComServer ComServer;
typedef struct Peer Peer;

enum CommsEventType {
	COMMS_NOTHING,
	COMMS_CONNECT,
	COMMS_DISCONNECT,
	COMMS_MESSAGE,
	COMMS_ERROR
};

struct CommsEvent {
	enum CommsEventType type;
	Peer *peer;
	int message_size;
	int error_code;
};

ComServer * spawnServer(int port);
Peer * connectToRemoteHost(const char *hostname, int port);

int sendMessageToPeer(Peer *peer, unsigned char *data, int datasize);
int getMessageFromPeer(Peer *peer, unsigned char *buf, int bufsize);

struct CommsEvent getServerEvent(ComServer *server);
struct CommsEvent getPeerEvent(Peer *peer);

void unspawnServer(ComServer *server);
void disconnectPeer(Peer *peer);
