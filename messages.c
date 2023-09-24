#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <messages.h>

int readU8(struct Slice *slice, unsigned char *u){
	if(slice->len < 1) return -1;
	*u = slice->ptr[0];
	slice->ptr += 1;
	slice->len -= 1;
	return 0;
}

int writeU8(struct Slice *buf, unsigned char u){
	if (buf->len < 1) return -1;
	buf->ptr[0] = u;
	buf->ptr++;
	buf->len--;
	return 1;
}

int readU16BE(struct Slice *slice, uint16_t *u){
	if(slice->len < 2) return -1;
	uint16_t a = slice->ptr[0];
	uint16_t b = slice->ptr[1];
	*u = a << 8 | b;
	slice->ptr += 2;
	slice->len -= 2;
	return 0;
}

int writeU16BE(struct Slice *buf, uint16_t u){
	if(buf->len < 2) return -1;
	buf->ptr[0] = (u >> 8) & 255;
	buf->ptr[1] = (u >> 0) & 255;
	buf->ptr += 2;
	buf->len -= 2;
	return 2;
}

int readU32BE(struct Slice *slice, uint32_t *u){
	if(slice->len < 4) return -1;
	uint32_t a = slice->ptr[0];
	uint32_t b = slice->ptr[1];
	uint32_t c = slice->ptr[2];
	uint32_t d = slice->ptr[3];
	*u = a << 24 | b << 16 | c << 8 | d;
	slice->ptr += 4;
	slice->len -= 4;
	return 0;
}

int writeU32BE(struct Slice *buf, uint32_t u){
	if(buf->len < 4) return -1;
	buf->ptr[0] = (u >> 24) & 255;
	buf->ptr[1] = (u >> 16) & 255;
	buf->ptr[2] = (u >> 8)  & 255;
	buf->ptr[3] = (u >> 0)  & 255;
	buf->ptr += 4;
	buf->len -= 4;
	return 4;
}

int readU64BE(struct Slice *slice, uint64_t *out){
	if(slice->len < 8) return -1;
	unsigned char *data = slice->ptr;
	uint64_t a = data[0];
	uint64_t b = data[1];
	uint64_t c = data[2];
	uint64_t d = data[3];
	uint64_t e = data[4];
	uint64_t f = data[5];
	uint64_t g = data[6];
	uint64_t h = data[7];
	*out =
		a << (7*8) | b << (6*8) | c << (5*8) | d << (4*8) |
		e << (3*8) | f << (2*8) | g << (1*8) | h << (0*8);
	slice->ptr += 8;
	slice->len -= 8;
	return 0;
}

int writeU64BE(struct Slice *buf, uint64_t n){
	if(buf->len < 8) return -1;
	buf->ptr[0] = (n >> 56) & 255;
	buf->ptr[1] = (n >> 48) & 255;
	buf->ptr[2] = (n >> 40) & 255;
	buf->ptr[3] = (n >> 32) & 255;
	buf->ptr[4] = (n >> 24) & 255;
	buf->ptr[5] = (n >> 16) & 255;
	buf->ptr[6] = (n >> 8)  & 255;
	buf->ptr[7] = (n >> 0)  & 255;
	buf->ptr += 8;
	buf->len -= 8;
	return 8;
}


int writeF64BE(struct Slice *buf, double x){
	if(x == 0.0){
		return writeU64BE(buf, 0);
	}

	int expo;
	double mant = frexp(x, &expo);

	uint64_t chunk1 = mant < 0 ? 1 : 0;
	uint64_t chunk2 = expo + 1023;
	uint64_t chunk3 = scalbn(fabs(mant), 52);

	uint64_t code = chunk1 << 63 | chunk2 << 52 | chunk3;

	return writeU64BE(buf, code);
}

int readF64BE(struct Slice *slice, double *out){
	if(slice->len < 8) return -1;

	uint64_t n;
	readU64BE(slice, &n);

	uint64_t sign   = (n >> 63) & 0x1UL;
	uint64_t chunk2 = (n >> 52) & 0x7ffUL;
	uint64_t chunk3 = (n >> 0)  & 0xfffffffffffffUL;

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



bool match(const char * string, const char * prefix){
	for (int i = 0; prefix[i]; i++) {
		if(string[i] != prefix[i]) return false;
	}
	return true;
}


int parseMessage(struct Slice * slice, const char * format, ...) {

	va_list list;

	int formatLength = strlen(format);

	int i = 0;

	va_start(list, format);

	while (i < formatLength && slice->len > 0) {
		if(format[i] == '{'){
			if (match(format + i, "{u8}")) {
				unsigned char * out = va_arg(list, unsigned char *);
				int status = readU8(slice, out); if (status < 0) return -1;
				i += 4;
			}
			else if (match(format + i, "{u16}")) {
				uint16_t * out = va_arg(list, uint16_t *);
				int status = readU16BE(slice, out); if (status < 0) return -1;
				i += 5;
			}
			else if (match(format + i, "{u32}")) {
				uint32_t * out = va_arg(list, uint32_t *);
				int status = readU32BE(slice, out); if (status < 0) return -1;
				i += 5;
			}
			else if (match(format + i, "{u64}")) {
				uint64_t * out = va_arg(list, uint64_t *);
				int status = readU64BE(slice, out); if (status < 0) return -1;
				i += 5;
			}
			else if (match(format + i, "{f64}")) {
				double * out = va_arg(list, double *);
				int status = readF64BE(slice, out); if (status < 0) return -1;
				i += 5;
			}
			else {
				fprintf(stderr, "parseMessage: bad format pattern\n");
				return -1;
			}
			// try to read fixed size component
		}
		else{
			if(slice->ptr[0] == format[i]){
				slice->ptr++;
				slice->len--;
				i++;
			}
			else{
				return -1;
			}
		}
	}

	va_end(list);

	if(i < formatLength){
		printf("ran out of input\n");
		return -1;
	}

	return 0;
}



int encodeMessage(struct Slice *buf, const char * format, ...) {

	va_list list;

	int formatLength = strlen(format);

	int i = 0;
	int bytes = 0;

	va_start(list, format);

	while (i < formatLength && buf->len > 0) {
		if(format[i] == '{'){
			if (match(format + i, "{u8}")) {
				int n = va_arg(list, int);
				int e = writeU8(buf, n); if (e < 0) return -1;
				i += 4;
				bytes += 1;
			}
			else if (match(format + i, "{u16}")) {
				int n = va_arg(list, int);
				int e = writeU16BE(buf, n); if (e < 0) return -1;
				i += 5;
				bytes += 2;
			}
			else if (match(format + i, "{u32}")) {
				/* what is going on here */
				uint32_t n = va_arg(list, uint32_t);
				int e = writeU32BE(buf, n); if (e < 0) return -1;
				i += 5;
				bytes += 4;
			}
			else if (match(format + i, "{u64}")) {
				uint64_t x = va_arg(list, uint64_t);
				int e = writeU64BE(buf, x); if (e < 0) return -1;
				i += 5;
				bytes += 8;
			}
			else if (match(format + i, "{f64}")) {
				double x = va_arg(list, double);
				int e = writeF64BE(buf, x); if (e < 0) return -1;
				i += 5;
				bytes += 8;
			}
			else {
				fprintf(stderr, "encodeMessage: bad format pattern\n");
				return -1;
			}
			// try to read fixed size component
		}
		else{
			buf->ptr[0] = format[i];
			buf->ptr++;
			buf->len--;
			i++;
			bytes++;
		}
	}

	va_end(list);

	if(i < formatLength){
		printf("ran out of buffer\n");
		return -1;
	}

	return bytes;

}

int unparsePing(struct Ping *ping, struct Slice *buf){
	return encodeMessage(buf, "PING|{u16}|{f64}",
		ping->sequence,
		ping->time
	);
}

int parsePing(struct Slice *buf, struct Ping *ping){

	uint16_t seq;
	double t;

	int err = parseMessage(buf, "PING|{u16}|{f64}", &seq, &t);

	if (err) return -1;

	if(seq > 1000) return -1;
	if(t < 0)      return -1;

	ping->sequence = seq;
	ping->time = t;

	return 0;
}

int unparsePong(struct Pong *pong, struct Slice *buf){
	return encodeMessage(buf, "PONG|{u16}|{f64}|{f64}",
		pong->sequence,
		pong->time1,
		pong->serverTime
	);
}

int parsePong(struct Slice *buf, struct Pong *pong){

	// parse raw data according to pattern
	uint16_t seq;
	double t, st;
	int err = parseMessage(buf, "PONG|{u16}|{f64}|{f64}", &seq, &t, &st);
	if (err) return -1;

	// reject invalid values
	if (seq > 1000) return -1;
	if (t < 0)      return -1;
	if (st < 0)     return -1;

	// construct valid record
	pong->sequence = seq;
	pong->time1 = t;
	pong->serverTime = st;

	return 0;
}
