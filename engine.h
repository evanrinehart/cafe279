struct Engine {
	bool paused;
	long long frameNumber;
	double serverTime;
	double localTime;
	double timeOffset;
	bool multiplayerEnabled;
	bool headless;
	enum {SERVER, CLIENT} multiplayerRole;
	char serverHostname[1024];
	int serverPort;
};

extern struct Engine engine;
