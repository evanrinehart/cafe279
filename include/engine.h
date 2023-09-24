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

	bool videoEnabled;
	bool vsync;
	bool vsyncHint;

	enum {OFFLINE, SERVER, CLIENT, CONNECTING} networkStatus;

	const char * finalMsg;
};

extern struct Engine engine;
