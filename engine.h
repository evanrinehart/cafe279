struct Engine {
	bool paused;
	long long frameNumber;
	double serverTime;
	double localTime;
	double timeOffset;
	bool graphical;
	bool dedicated;
	char serverHostname[1024];
	int serverPort;
	bool shouldClose;
	bool inputFresh;

	enum {OFFLINE, SERVER, CLIENT, CONNECTING} networkStatus;
};

extern struct Engine engine;
