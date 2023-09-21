#define FFSIZE 256

int flood(
	int i, int j,
	char horiz[FFSIZE][FFSIZE],
	char vert[FFSIZE][FFSIZE],
	void* userdata,
	int (*visit)(void* userdata, int i, int j)
);
