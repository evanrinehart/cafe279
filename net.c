#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <math.h>
#include <limits.h>

#include <threads.h>
#include <net.h>


int UDPClient(const char * host, int port, struct Mailbox *mb);
int UDPServer(int port, struct Mailbox *m);

int sendMessageToClient(struct Mailbox *m);
int sendMessageToServer(struct Mailbox *m);
int waitForInbox(struct Mailbox *m);

struct Mailbox * mallocMailbox();
void wipeMailbox(struct Mailbox *m);
void closeMailbox(struct Mailbox *m);
void cloneMailbox(struct Mailbox *dst, struct Mailbox *src);
void printMailbox(struct Mailbox *m);

void scribble8(struct Mailbox *m, unsigned char c);
void scribbleChar(struct Mailbox *m, char c);
void scribbleString(struct Mailbox *m, const char *str);
void scribble16(struct Mailbox *m, unsigned int n);
void scribble32(struct Mailbox *m, unsigned int n);
void scribble64(struct Mailbox *m, unsigned long long n);
void scribbleFloat(struct Mailbox *m, float x);
void scribbleDouble(struct Mailbox *m, double x);

int portFromAddr(struct sockaddr_storage *addr, socklen_t len);
int portFromSocket(int socket);
void printMessage(unsigned char buf[], int n);
const char * addressToString(struct sockaddr_storage *ss, int *port);
int compareAddress(struct sockaddr_storage *ssA, struct sockaddr_storage *ssB);

/* misc */

struct sockaddr_in localhostPort(int port){
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	return addr;
}

/* MAILBOX */

void wipeMailbox(struct Mailbox *m){
	m->socket = 0;
	m->error_number = 0;
	m->datasize = 0;
	m->addrlen = 0;
	bzero(&m->addr, sizeof m->addr);
	m->data[0] = 0;
}


void closeMailbox(struct Mailbox *m){

	int e = close(m->socket);

	if(e < 0){
		fprintf(stderr, "close: %s\n", strerror(errno));
	}

	m->socket = 0;

}

void cloneMailbox(struct Mailbox *dst, struct Mailbox *src){
	dst->socket = src->socket;
	dst->addrlen = src->addrlen;
	dst->datasize = 0;
	memcpy(&dst->addr, &src->addr, src->addrlen);
}

void printMailbox(struct Mailbox *m){
	printf("Mailbox\n");
	printf("  socket = %d\n", m->socket);
	int port;
	const char *ip = addressToString(&m->addr, &port);
	printf("  addr = %s %d\n", ip, port);
	printf("  error_number = %d\n", m->error_number);
	printf("  datasize = %d\n", m->datasize);
	printf("  data[%lu] =\n", sizeof m->data);
	printMessage(m->data, m->datasize);
}

struct Mailbox * mallocMailbox(){
	struct Mailbox *m = malloc(sizeof (struct Mailbox));
	wipeMailbox(m);
	return m;
}

void scribble8(struct Mailbox *m, unsigned char c){
	if(m->datasize >= sizeof m->data){
		fprintf(stderr, "scribble8: buffer full\n");
		return;
	}
	m->data[m->datasize++] = c;
}

void scribbleChar(struct Mailbox *m, char c){
	if(m->datasize >= sizeof m->data){
		fprintf(stderr, "scribbleChar: buffer full\n");
		return;
	}
	scribble8(m, c);
}

void scribbleString(struct Mailbox *m, const char *str){
	int space = sizeof m->data - m->datasize;
	int strsize = strlen(str);
	if(strsize <= space) {
		memcpy(m->data + m->datasize, str, strsize);
		m->datasize += strsize;
	}
	else {
		fprintf(stderr, "scribbleString: buffer full\n");
		memcpy(m->data + m->datasize, str, space);
		m->datasize = 0;
	}
}

void scribble16(struct Mailbox *m, unsigned int n){
	int space = MAILBOX_SIZE - m->datasize;
	if(space < 2){ fprintf(stderr, "scribble16: buffer full\n"); return; }
	m->data[m->datasize++] = (n >> 8) & 255;
	m->data[m->datasize++] = (n >> 0) & 255;
}

void scribble32(struct Mailbox *m, unsigned int n){
	int space = MAILBOX_SIZE - m->datasize;
	if(space < 4){ fprintf(stderr, "scribble32: buffer full\n"); return; }
	m->data[m->datasize++] = (n >> 24) & 255;
	m->data[m->datasize++] = (n >> 16) & 255;
	m->data[m->datasize++] = (n >> 8)  & 255;
	m->data[m->datasize++] = (n >> 0)  & 255;
}

void scribble64(struct Mailbox *m, unsigned long long n){
	int space = MAILBOX_SIZE - m->datasize;
	if(space < 8){ fprintf(stderr, "scribble64: buffer full\n"); return; }
	m->data[m->datasize++] = (n >> 56) & 255;
	m->data[m->datasize++] = (n >> 48) & 255;
	m->data[m->datasize++] = (n >> 40) & 255;
	m->data[m->datasize++] = (n >> 32) & 255;
	m->data[m->datasize++] = (n >> 24) & 255;
	m->data[m->datasize++] = (n >> 16) & 255;
	m->data[m->datasize++] = (n >> 8)  & 255;
	m->data[m->datasize++] = (n >> 0)  & 255;
}

unsigned long long unscribble64(unsigned char data[8]){
	unsigned long long c;
	unsigned long long n = 0;
	c = data[0]; n |= c << (7 * 8);
	c = data[1]; n |= c << (6 * 8);
	c = data[2]; n |= c << (5 * 8);
	c = data[3]; n |= c << (4 * 8);
	c = data[4]; n |= c << (3 * 8);
	c = data[5]; n |= c << (2 * 8);
	c = data[6]; n |= c << (1 * 8);
	c = data[7]; n |= c << (0 * 8);
	return n;
}

void scribbleFloat(struct Mailbox *m, float x){
	int space = MAILBOX_SIZE - m->datasize;
	if(space < 4){ fprintf(stderr, "scribbleFloat: buffer full\n"); return; }

	int expo;
	float mantissa = frexpf(x, &expo);

	unsigned chunk1 = mantissa < 0 ? 1 : 0;
	unsigned chunk2 = expo + 127;
	unsigned chunk3 = scalbnf(fabsf(mantissa), 23);

	unsigned code = chunk1 << 31 | chunk2 << 23 | chunk3;

	scribble32(m, code);
}

// scribble 8 bytes on the outgoing mailbox buffer
void scribbleDouble(struct Mailbox *m, double x){
	int space = MAILBOX_SIZE - m->datasize;
	if(space < 8){ fprintf(stderr, "scribbleDouble: buffer full\n"); return; }

	int expo;
	double mant = frexp(x, &expo);

	unsigned long long chunk1 = mant < 0 ? 1 : 0;
	unsigned long long chunk2 = expo + 1023;
	unsigned long long chunk3 = scalbn(fabs(mant), 52);

	unsigned long long code = chunk1 << 63 | chunk2 << 52 | chunk3;

	scribble64(m, code);
}

double unscribbleDouble(unsigned char data[8]){

	unsigned long long n = unscribble64(data);

	unsigned long long sign   = (n >> 63) & 0x1UL;
	unsigned long long chunk2 = (n >> 52) & 0x7ffUL;
	unsigned long long chunk3 = (n >> 0)  & 0xfffffffffffffUL;

	int expo = chunk2 - 1023;
	double mant = chunk3;
	mant = scalbn(mant, -52);
	double x = ldexp(mant, expo);

	return sign ? -x : x;

}


int sendMessageToClient(struct Mailbox *m){

	int num_written = sendto(
		m->socket,
		m->data,
		m->datasize,
		0,
		(struct sockaddr*)&m->addr,
		m->addrlen
	);

	if (num_written < 0) {
		// EAGAIN EWOULDBLOCK
		// EAGAIN
		// EBADF
		// EFAULT
		// EINTR - signal occurred before data was transmitted
		// EMSGSIZE - message too big
		// ENOTSOCK
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		m->error_number = errno;
		return -1;
	}

	if (num_written < m->datasize) {
		fprintf(stderr, "sendto sent fewer bytes than requested %d / %d\n",
			num_written, m->datasize);
		m->error_number = 0;
		return -1;
	}

	m->datasize = 0;

	return 0;
}

int sendMessageToServer(struct Mailbox *m){

	int num_written = send(m->socket, m->data, m->datasize, 0);

	if (num_written < 0) {
		// EAGAIN EWOULDBLOCK
		// EAGAIN
		// EBADF
		// EFAULT
		// EINTR - signal occurred before data was transmitted
		// EMSGSIZE - message too big
		// ENOTSOCK
		fprintf(stderr, "sendto: %s\n", strerror(errno));
		m->error_number = errno;
		return -1;
	}

	if (num_written < m->datasize) {
		fprintf(stderr, "sendto sent fewer bytes than requested %d / %d\n",
			num_written, m->datasize);
		m->error_number = 0;
		return -1;
	}

	m->datasize = 0;

	return 0;
}



// create a UDP socket and bind it for listening
int UDPServer(int port, struct Mailbox *m){

	struct addrinfo *res;
	struct addrinfo hints;

	hints.ai_family = AF_UNSPEC;
	//hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char service[64]; sprintf(service, "%d", port);

	int err = getaddrinfo(NULL, service, &hints, &res);

	if(err){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	for (struct addrinfo *ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		int s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s < 0) {
			fprintf(stderr, "socket: %s\n", strerror(errno));
			continue;
		}

		err = bind(s, ptr->ai_addr, ptr->ai_addrlen);
		
		if (err < 0) {
			fprintf(stderr, "bind: %s\n", strerror(errno));
			close(s);
			continue;
		}

		m->socket = s;
		m->port = port;
		memcpy(&m->addr, ptr->ai_addr, ptr->ai_addrlen);
		m->addrlen = ptr->ai_addrlen;
		m->error_number = 0;
		m->stopflag = false;
		m->dataflag = false;
		m->datasize = 0;

		freeaddrinfo(res);

		return 0;
	}

	freeaddrinfo(res);

	fprintf(stderr, "getaddrinfo: no suitable server address\n");

	return -1;
}

int UDPClient(const char * host, int port, struct Mailbox *mb){
	struct addrinfo *res;
	struct addrinfo hints;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	char service[64]; sprintf(service, "%d", port);

	int err = getaddrinfo(host, service, &hints, &res);

	if(err){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	for (struct addrinfo *ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		int s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s < 0) {
			fprintf(stderr, "socket: %s\n", strerror(errno));
			continue;
		}

		err = connect(s, ptr->ai_addr, ptr->ai_addrlen);
		
		if (err < 0) {
			fprintf(stderr, "connect: %s\n", strerror(errno));
			close(s);
			continue;
		}

		mb->socket = s;
		mb->port = portFromSocket(s);
		memcpy(&mb->addr, ptr->ai_addr, ptr->ai_addrlen);
		mb->addrlen = ptr->ai_addrlen;
		mb->datasize = 0;
		mb->error_number = 0;

		freeaddrinfo(res);

		return 0;
	}

	freeaddrinfo(res);

	fprintf(stderr, "getaddrinfo: could not bind socket\n");

	return -1;
}

const char * addressToString(struct sockaddr_storage *ss, int *port){
	static char bigbuf[1024];
	char smallbuf[64];

	int e = getnameinfo(
		(struct sockaddr *)ss,
		sizeof (struct sockaddr_storage),
		bigbuf, 1024,
		smallbuf, 64,
		NI_NUMERICHOST | NI_NUMERICSERV
	);

	if(e < 0){
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(e));
		strcpy(bigbuf, "(getnameinfo error)");
		*port = 0;
		return bigbuf;
	}

	*port = atoi(smallbuf);
	return bigbuf;
}

int compareAddress(struct sockaddr_storage *ssA, struct sockaddr_storage *ssB){
	int famA = ssA->ss_family;
	int famB = ssB->ss_family;

	if(famA != famB) return famA - famB;

	if(famA == AF_INET) {
		struct sockaddr_in *inA = (struct sockaddr_in *)ssA;
		struct sockaddr_in *inB = (struct sockaddr_in *)ssB;
		int portA = inA->sin_port;
		int portB = inB->sin_port;
		if(portA != portB) return portA - portB;
		else return memcmp(&inA->sin_addr, &inB->sin_addr, sizeof (struct in_addr));
	}
	else if(famA == AF_INET6){
		struct sockaddr_in6 *inA = (struct sockaddr_in6 *)ssA;
		struct sockaddr_in6 *inB = (struct sockaddr_in6 *)ssB;
		int portA = inA->sin6_port;
		int portB = inB->sin6_port;
		if(portA != portB) return portA - portB;
		else return memcmp(&inA->sin6_addr, &inB->sin6_addr, sizeof (struct in6_addr));
	}
	else{
		fprintf(stderr, "compareAddress: unknown address family\n");
		exit(1);
	}
}


int waitForInbox(struct Mailbox *m){

	int num_read = recvfrom(
		m->socket,
		m->data,
		sizeof m->data,
		0,
		(struct sockaddr*)&m->addr,
		&m->addrlen
	);

	if(num_read < 0){
		fprintf(stderr,"recvfrom (waitForInbox): %s\n", strerror(errno));
		m->error_number = errno;
		return -1;
	}

	m->datasize = num_read;

	return 0;

}

int portFromAddr(struct sockaddr_storage *addr, socklen_t len){
	in_port_t nport;
	switch(addr->ss_family){
		case AF_INET:  nport = ((struct sockaddr_in  *)addr)->sin_port; break;
		case AF_INET6: nport = ((struct sockaddr_in6 *)addr)->sin6_port; break;
		default:       nport = 0;
	}
	return ntohs(nport);
}

int portFromSocket(int socket){
	struct sockaddr_storage addr;
	socklen_t addrlen = sizeof addr;

	int e = getsockname(socket, (struct sockaddr*)&addr, &addrlen);

	if(e < 0){
		printf("getsockname: %s\n", strerror(errno));
		return -1;
	}

	return portFromAddr(&addr, addrlen);
}
