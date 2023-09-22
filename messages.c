#include <string.h>
#include <stdio.h>
#include <math.h>

struct Ping {
	unsigned sequence;
	double time;
};

struct Pong {
	unsigned sequence;
	double time1;
	double serverTime;
};

struct Slice {
	unsigned char * ptr;
	int len;
};


int readU8(struct Slice *slice, unsigned *u){
	if(slice->len < 1) return -1;
	*u = slice->ptr[0];
	slice->ptr += 1;
	slice->len -= 1;
	return 0;
}

void writeU8(unsigned char *buf, unsigned u){
	buf[0] = u;
}

int readU16BE(struct Slice *slice, unsigned *u){
	if(slice->len < 2) return -1;
	*u = 0;
	*u |= slice->ptr[1] << 0;
	*u |= slice->ptr[0] << 8;
	slice->ptr += 2;
	slice->len -= 2;
	return 0;
}

void writeU16BE(unsigned char *buf, unsigned u){
	buf[0] = (u >> 8) & 255;
	buf[1] = (u >> 0) & 255;
}

int readU32BE(struct Slice *slice, unsigned *u){
	if(slice->len < 4) return -1;
	*u = 0;
	*u |= slice->ptr[3] << 0;
	*u |= slice->ptr[2] << 8;
	*u |= slice->ptr[1] << 16;
	*u |= slice->ptr[0] << 24;
	slice->ptr += 4;
	slice->len -= 4;
	return 0;
}

void writeU32BE(unsigned char *buf, unsigned u){
	buf[0] = (u >> 24) & 255;
	buf[1] = (u >> 16) & 255;
	buf[2] = (u >> 8)  & 255;
	buf[3] = (u >> 0)  & 255;
}

int readU64BE(struct Slice *slice, unsigned long long *out){
	if(slice->len < 8) return -1;
	unsigned long long c;
	unsigned long long n = 0;
	unsigned char *data = slice->ptr;
	c = data[0]; n |= c << (7 * 8);
	c = data[1]; n |= c << (6 * 8);
	c = data[2]; n |= c << (5 * 8);
	c = data[3]; n |= c << (4 * 8);
	c = data[4]; n |= c << (3 * 8);
	c = data[5]; n |= c << (2 * 8);
	c = data[6]; n |= c << (1 * 8);
	c = data[7]; n |= c << (0 * 8);
	*out = n;
	slice->ptr += 8;
	slice->len -= 8;
	return 0;
}

void writeU64BE(unsigned char *buf, unsigned long long n){
	buf[0] = (n >> 56) & 255;
	buf[1] = (n >> 48) & 255;
	buf[2] = (n >> 40) & 255;
	buf[3] = (n >> 32) & 255;
	buf[4] = (n >> 24) & 255;
	buf[5] = (n >> 16) & 255;
	buf[6] = (n >> 8)  & 255;
	buf[7] = (n >> 0)  & 255;
}


void writeF64BE(unsigned char * buf, double x){
	int expo;
	double mant = frexp(x, &expo);

	unsigned long long chunk1 = mant < 0 ? 1 : 0;
	unsigned long long chunk2 = expo + 1023;
	unsigned long long chunk3 = scalbn(fabs(mant), 52);

	unsigned long long code = chunk1 << 63 | chunk2 << 52 | chunk3;

	writeU64BE(buf, code);
}

int readF64BE(struct Slice *slice, double *out){
	if(slice->len < 8) return -1;

	unsigned long long n;
	readU64BE(slice, &n);

	unsigned long long sign   = (n >> 63) & 0x1UL;
	unsigned long long chunk2 = (n >> 52) & 0x7ffUL;
	unsigned long long chunk3 = (n >> 0)  & 0xfffffffffffffUL;

	int expo = chunk2 - 1023;
	double mant = chunk3;
	mant = scalbn(mant, -52);
	double x = ldexp(mant, expo);

	*out = sign ? -x : x;
	return 0;
}

int readChars(struct Slice *slice, const char * string){
	int n = strlen(string);
	if(slice->len < n) return -1;
	if(memcmp(slice->ptr, string, n) != 0) return -1;
	slice->ptr += n;
	slice->len -= n;
	return 0;
}

int parsePing(unsigned char * buf, int bufsize, struct Ping *ping){
	struct Slice slice = {buf, bufsize};
	int err;
	err = readChars(&slice, "PING\n");        if (err) { return -1; }
	err = readU32BE(&slice, &ping->sequence); if (err) { return -1; }
	err = readChars(&slice, "\n");            if (err) { return -1; }
	err = readF64BE(&slice, &ping->time);     if (err) { return -1; }
	err = readChars(&slice, "\n");            if (err) { return -1; }

	if(ping->sequence > 1000) { return -1; }
	if(ping->time < 0) { return -1; }

	return 0;
}

int unparsePing(struct Ping *ping, unsigned char *buf, int bufsize){
	unsigned char *pen = buf;
	const int size = 4 + 1 + 4 + 1 + 8 + 1;
	if(bufsize < size) return -1;
	memcpy(pen, "PING", 4);          pen += 4;
	*pen = '\n';                     pen += 1;
	writeU32BE(pen, ping->sequence); pen += 4;
	*pen = '\n';                     pen += 1;
	writeF64BE(pen, ping->time);     pen += 8;
	*pen = '\n';                     pen += 1;
	return size;
}


int unparsePong(struct Pong *pong, unsigned char *buf, int bufsize){
	unsigned char *pen = buf;
	const int size = 4 + 1 + 4 + 1 + 8 + 1 + 8 + 1;
	if(bufsize < size) return -1;
	memcpy(pen, "PONG", 4);             pen += 4;
	*pen = '\n';                        pen += 1;
	writeU32BE(pen, pong->sequence);    pen += 4;
	*pen = '\n';                        pen += 1;
	writeF64BE(pen, pong->time1);       pen += 8;
	*pen = '\n';                        pen += 1;
	writeF64BE(pen, pong->serverTime);  pen += 8;
	*pen = '\n';                        pen += 1;
	return size;
}

int parsePong(unsigned char * buf, int bufsize, struct Pong *pong){
	struct Slice slice = {buf, bufsize};
	int err;
	err = readChars(&slice, "PONG\n");          if (err) { return -1; }
	err = readU32BE(&slice, &pong->sequence);   if (err) { return -1; }
	err = readChars(&slice, "\n");              if (err) { return -1; }
	err = readF64BE(&slice, &pong->time1);      if (err) { return -1; }
	err = readChars(&slice, "\n");              if (err) { return -1; }
	err = readF64BE(&slice, &pong->serverTime); if (err) { return -1; }
	err = readChars(&slice, "\n");              if (err) { return -1; }

	if(pong->sequence > 1000) { return -1; }
	if(pong->time1 < 0) { return -1; }
	if(pong->serverTime < 0) { return -1; }

	return 0;
}

#include <ctype.h>
void printMessage(unsigned char buf[], int n){
	for(int i = 0; i < n; i++){
		unsigned char byte = buf[i];
		if(isprint(byte) && byte != ' '){
			printf("%c ", byte);
		}
		else{
			printf("#%u ", byte);
		}
	}

	puts("");
}



static void test(void){

	struct Ping ping = {133, 3.14159};

	unsigned char buf[2048];

	int e = unparsePing(&ping, buf, 2048);

	if(e < 0){ printf("unparse failed\n"); return; }

	const int size = 4 + 1 + 4 + 1 + 8 + 1;

	printMessage(buf, size);

	bzero(&ping, sizeof ping);

	e = parsePing(buf, 1024, &ping);

	if(e < 0){ printf("parse failed\n"); return; }

	printf("Ping %u %lf\n", ping.sequence, ping.time);


	struct Pong pong = {ping.sequence, ping.time, 7.70717};
	e = unparsePong(&pong, buf, 1024);
	if(e < 0){ printf("unparse failed\n"); return; }
	printMessage(buf, size);
	bzero(&pong, sizeof pong);
	e = parsePong(buf, 1024, &pong);
	if(e < 0){ printf("parse failed\n"); return; }
	printf("Pong %u %lf %lf\n", pong.sequence, pong.time1, pong.serverTime);
}

