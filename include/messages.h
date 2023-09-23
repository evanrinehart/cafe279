struct Slice {
	unsigned char * ptr;
	int len;
};

struct Ping {
	int sequence;
	double time;
};

struct Pong {
	int sequence;
	double time1;
	double serverTime;
};

int unparsePing(struct Ping *ping, struct Slice *buf);
int parsePing(struct Slice *buf, struct Ping *ping);
int unparsePong(struct Pong *pong, struct Slice *buf);
int parsePong(struct Slice *buf, struct Pong *pong);

