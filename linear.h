
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

#define vec2(X,Y) ((vec2){X,Y})

int vec2eq(vec2 a, vec2 b);
int vec2neq(vec2 a, vec2 b);

struct vec2 add(struct vec2 a, struct vec2 b);
struct vec2 sub(struct vec2 a, struct vec2 b);
struct vec2 neg(struct vec2 a);
struct vec2 scale(double s, struct vec2 u);
double dot(struct vec2 a, struct vec2 b);
double cross(struct vec2 a, struct vec2 b);
double normSquared(struct vec2 v);
double norm(struct vec2 v);
vec2 normalize(struct vec2 v);
double angle(struct vec2 a, struct vec2 b);
struct vec2 rotate(struct vec2 u, double angle);
vec2 cis(double angle);

struct vec2 normal(struct vec2 a, struct vec2 b);
struct vec2 project(struct vec2 a, struct vec2 b);
struct vec2 reflect(struct vec2 a, struct vec2 L);

double distanceSquaredTo(vec2 a, vec2 b);

struct vec2 lerp(struct vec2 a, struct vec2 b, double t);

void abcForm(struct vec2 p0, struct vec2 p1, double abcOut[3]);
int solve2x2(double abc0[3], double abc1[3], struct vec2 *out);
struct aabb segmentAABB(struct vec2 p0, struct vec2 p1);
struct aabb aabbIntersection(struct aabb A, struct aabb B);
int pointInAABB(struct vec2 p, struct aabb box);
int aabbApart(struct aabb A, struct aabb B);

int segmentsCross(struct vec2 p0, struct vec2 p1, struct vec2 q0, struct vec2 q1);
int segmentsIntersection(struct vec2 p0, struct vec2 p1, struct vec2 q0, struct vec2 q1, vec2 *crossPointOut);
