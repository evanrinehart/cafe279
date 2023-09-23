enum SyncStatus {
	SYNC_INACTIVE,
	SYNC_WORKING
};

struct SyncCallbacks {
	void (*complete)(double offset);
	void (*failed)(void);
};

struct Sample {
	double time1;
	double serverTime;
	double time2;
	double rtt;
	bool blank;
};

extern enum SyncStatus syncStatus;

void syncBegin(struct SyncCallbacks cbs);
void syncPoll();
void syncPong(int sequence, double time1, double serverTime, double time2);

void printSamples();
