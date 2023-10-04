
struct aabb {
	double x0;
	double x1;
	double y0;
	double y1;
};

struct vec2 {
	double x;
	double y;
};

typedef struct vec2 vec2;

struct Vec2Pair {
	vec2 a;
	vec2 b;
};

struct fix8 {
	int to_i;
};

typedef struct fix8 fix8;

#define fix8(X) ((fix8){X})


#define vec2(X,Y) ((vec2){X,Y})

int vec2eq(vec2 a, vec2 b);
int vec2neq(vec2 a, vec2 b);

vec2 add(vec2 a, vec2 b);
vec2 sub(vec2 a, vec2 b);
vec2 neg(vec2 a);
vec2 scale(double s, vec2 u);
double dot(vec2 a, vec2 b);
double cross(vec2 a, vec2 b);
double normSquared(vec2 v);
double norm(vec2 v);
vec2 normalize(vec2 v);
double angle(vec2 a, vec2 b);
vec2 rotate(vec2 u, double angle);
vec2 cis(double angle);
vec2 rcis(double r, double angle);

vec2 normal(vec2 a, vec2 b);
vec2 projection(vec2 a, vec2 b);
vec2 reflection(vec2 a, vec2 L);
vec2 rejection(vec2 a, vec2 b);

double distanceSquaredTo(vec2 a, vec2 b);

vec2 lerp(vec2 a, vec2 b, double t);

void abcForm(vec2 p0, vec2 p1, double abcOut[3]);
int solve2x2(double abc0[3], double abc1[3], vec2 *out);
int quadraticFormula(double A, double B, double C, double *answer);
struct aabb segmentAABB(vec2 p0, vec2 p1);
struct aabb aabbIntersection(struct aabb A, struct aabb B);
int pointInAABB(vec2 p, struct aabb box);
int aabbApart(struct aabb A, struct aabb B);

int segmentsCross(vec2 p0, vec2 p1, vec2 q0, vec2 q1);
int segmentsIntersection(vec2 p0, vec2 p1, vec2 q0, vec2 q1, vec2 *crossPointOut);


