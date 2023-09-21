struct Ping {
	int sequence;
	double time;
};

struct Pong {
	int sequence;
	double time1;
	double time2;
};

int parsePing(unsigned char * buf, int bufsize, struct Ping *ping);
int parsePong(unsigned char * buf, int bufsize, struct Pong *pong);
int unparsePing(struct Ping *ping, unsigned char *buf, int bufsize);
int unparsePong(struct Pong *pong, unsigned char *buf, int bufsize);
