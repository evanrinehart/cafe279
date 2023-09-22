// Base 10 proper fraction. Components never have opposite signs.
struct Nano {
	long long units;
	long nanos;
};

// a minus b
struct Nano nanodiff(struct Nano a, struct Nano b);

const char * nanoToString(struct Nano n);

void setStartTime(struct Nano t_zero);

// Query the system highres monotonic clock.
// Returns amount of time since some unspecified point in the past.
// Use setStartTime to set a value that will be automatically subtracted.
struct Nano chron();

// Get a chron() value approximated and returned in a double.
// Degrades to nanosecond precision at +52 days
// Degrades to microsecond precision at +68 years
// Before about 52 days the result rounds to the same number as chron()
double chronf();

// Value of chron() reduced to single int.
// Overflows around time +292 years
long long chronl();

void millisleep(int n);
