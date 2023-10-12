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

void millisleep(int n){
	usleep(n * 1000);
}


long getMillisecondsPastMidnightUTCPlus(int hours) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	long million = 1000 * 1000;
	long seconds = (ts.tv_sec + hours * 3600) % 86400;
	long millis  = ts.tv_nsec / million;

	return seconds * 1000 + millis;
}

long getInternetTimePrecise() {

	long ms = getMillisecondsPastMidnightUTCPlus(1);
	long beats = ms / 86400;
	long parts = ms % 86400;
	long milli = 1000 * parts / 86400;

	return beats * 1000 + milli;

}

int getInternetTime() {

	time_t t = time(NULL);
	struct tm * broken = gmtime(&t);

	if(broken == NULL){
		printf("gmtime_r failed\n");
		return 0;
	}

	printf(
		"(struct tm){sec=%d,min=%d,hour=%d,mday=%d,mon=%d,year=%d-1900,wday=%d,yday=%d,isdst=%d}\n",
		broken->tm_sec,
		broken->tm_min,
		broken->tm_hour,
		broken->tm_mday,
		broken->tm_mon,
		broken->tm_year + 1900,
		broken->tm_wday,
		broken->tm_yday,
		broken->tm_isdst
	);

	int hours   = (broken->tm_hour + 1) % 24;
	int minutes = broken->tm_min;
	int seconds = broken->tm_sec;

	int total = hours * 3600 + minutes * 60 + seconds;

	return 10 * total / 864;

}

static void test(){
/*
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

*/
}
