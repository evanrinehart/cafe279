#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <clocks.h>

static struct Nano start_time = {0, 0};

const char * nanoToString(struct Nano n){
	static char buf[64];
	if(n.nanos < 0) sprintf(buf, "-%lld.%09ld", -n.units, -n.nanos);
	else sprintf(buf, "%lld.%09ld", n.units, n.nanos);
	return buf;
}

// query the system highres monotonic clock.
// amount of time since some unspecified point in the past
struct Nano chron(){
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	struct Nano n = {tp.tv_sec, tp.tv_nsec};
	return nanodiff(n, start_time);
}

// approximate double-precision value of chrono()
double chronf(){
	struct Nano n = chron();
	return n.units + 1e-9 * n.nanos;
}

long long chronl(){
	const long long billion = 1000 * 1000 * 1000;
	struct Nano n = chron();
	return billion * n.units + n.nanos;
}

struct Nano nanodiff(struct Nano a, struct Nano b){
	// (B*a + b) - (B*c + d) = B*(a - c) + (b - d)

	const long long billion = 1000 * 1000 * 1000;

	struct Nano n = {a.units - b.units, a.nanos - b.nanos};

	if(n.nanos >= billion){
		n.units += 1;
		n.nanos -= billion;
	}
	else if(n.nanos <= -billion){
		n.units -= 1;
		n.nanos += billion;
	}

	if(n.units < 0 && n.nanos > 0){
		n.units += 1;
		n.nanos -= billion;
	}
	else if(n.units > 0 && n.nanos < 0){
		n.units -= 1;
		n.nanos += billion;
	}

	return n;
}

void setStartTime(struct Nano t_zero){
	start_time = t_zero;
}

static void test(){

	setStartTime(chron());

	struct Nano n1 = chron();
	printf("n1 = %s\n", nanoToString(n1));

	double m1 = n1.units + 1e-9 * n1.nanos;
	printf("m1 = %.18lf\n", m1);

	struct Nano n2 = chron();
	printf("n2 = %s\n", nanoToString(n2));

	double m2 = n2.units + 1e-9 * n2.nanos;
	printf("m2 = %.18lf\n", m2);

	double f3 = chronf();
	printf("f3 = %.18lf\n", f3);

	struct Nano d = nanodiff(n1, n2);
	printf("n2-n1 = %s\n", nanoToString(d));

	double e = (5e6 + m2) - (5e6 + m1);
	printf("m2-m1 = %.18lf\n", e);
}

void main(){ test(); }
