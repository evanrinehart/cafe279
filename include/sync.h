enum SyncStatus { SYNC_INACTIVE, SYNC_WORKING, SYNC_READY };

extern enum SyncStatus syncStatus;

struct Sample {
	double time1;
	double serverTime;
	double time2;
	double rtt;
	bool blank;
};

void printSamples();
int sendPing(int sequence, double time);
void syncPoll();
void syncPong(int sequence, double time1, double serverTime, double time2);
void syncBegin();
double syncEnd();
