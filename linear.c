#include <math.h>
#include <stdio.h>

#include <linear.h>

#define det(a,b,c,d) (a*d - b*c)
#define min(a,b) (a < b ? a : b)
#define max(a,b) (a < b ? b : a)
#define aabbOverlap(a,b) (!aabbApart(a,b))

int vec2eq(vec2 a, vec2 b){
	return a.x == b.x && a.y == b.y;
}

int vec2neq(vec2 a, vec2 b){
	return a.x != b.x || a.y != b.y;
}

struct vec2 add(struct vec2 a, struct vec2 b){
	return (struct vec2){a.x + b.x, a.y + b.y};
}

struct vec2 sub(struct vec2 a, struct vec2 b){
	return (struct vec2){a.x - b.x, a.y - b.y};
}

struct vec2 neg(struct vec2 a){
	return (struct vec2){-a.x, -a.y};
}

struct vec2 scale(double s, struct vec2 u){
	return (struct vec2){s * u.x, s * u.y};
}

double dot(struct vec2 a, struct vec2 b){
	return a.x * b.x + a.y * b.y;
}

double cross(struct vec2 a, struct vec2 b){
	return a.x * b.y - a.y * b.x;
}

double normSquared(struct vec2 v){
	return dot(v,v);
}

double norm(struct vec2 v){
	return sqrt(normSquared(v));
}

vec2 normalize(struct vec2 v){
	return scale(1.0 / norm(v), v);
}

double angle(struct vec2 a, struct vec2 b){
	double A = norm(a);
	double B = norm(b);
	return acos(dot(a,b) / (A * B));
}

struct vec2 rotate(struct vec2 v, double angle){
	struct vec2 out;
	if(angle == 0.0) return v;
	out.x = v.x * cos(angle) - v.y * sin(angle);
	out.y = v.x * sin(angle) + v.y * cos(angle);
	return out;
}

vec2 cis(double angle){
	vec2 v = {cos(angle), sin(angle)}; return v;
}

vec2 rcis(double r, double angle){
	vec2 v = {r * cos(angle), r * sin(angle)}; return v;
}

struct vec2 normal(struct vec2 a, struct vec2 b){
	struct vec2 in  = sub(b,a);
	struct vec2 out = {-in.y, in.x};
	return out;
}

struct vec2 project(struct vec2 a, struct vec2 b){
	return scale(dot(a,b)/dot(b,b), b);
}

struct vec2 reflect(struct vec2 a, struct vec2 L){
	return sub(scale(2, project(a, L)), a);
}

double distanceSquaredTo(vec2 a, vec2 b){
	return normSquared(sub(a, b));
}

struct vec2 lerp(struct vec2 a, struct vec2 b, double t){
	if(t == 0.0) return a;
	if(t == 1.0) return b;
	return add(a, scale(t, sub(b, a)));
}


void abcForm(struct vec2 p0, struct vec2 p1, double abcOut[3]){
	double x0 = p0.x;
	double y0 = p0.y;
	double x1 = p1.x;
	double y1 = p1.y;
	double A = y1 - y0;
	double B = x0 - x1;
	abcOut[0] = A;
	abcOut[1] = B;
	abcOut[2] = A*x0 + B*y0;
}

int solve2x2(double abc0[3], double abc1[3], struct vec2 *out){
	double a0 = abc0[0];
	double b0 = abc0[1];
	double c0 = abc0[2];
	double a1 = abc1[0];
	double b1 = abc1[1];
	double c1 = abc1[2];

	double D = det(a0,b0,a1,b1);

	//if(-0.00001 < D && D < 0.00001) return 1; /* no dice */
	if(D == 0.0) return 1; /* don't try to divide by zero */

	out->x = det(c0,c1,b0,b1) / D; /* "ol' cramer" */
	out->y = det(a0,a1,c0,c1) / D;

	return 0;
}

struct aabb segmentAABB(struct vec2 p0, struct vec2 p1){
	double x0 = p0.x;
	double y0 = p0.y;
	double x1 = p1.x;
	double y1 = p1.y;
	struct aabb out;
	out.x0 = min(x0,x1);
	out.x1 = max(x0,x1);
	out.y0 = min(y0,y1);
	out.y1 = max(y0,y1);
	return out;
}

/* undefined if A and B are apart */
struct aabb aabbIntersection(struct aabb A, struct aabb B){
	struct aabb out;
	out.x0 = max(A.x0, B.x0);
	out.x1 = min(A.x1, B.x1);
	out.y0 = max(A.y0, B.y0);
	out.y1 = min(A.y1, B.y1);
	return out;
}

/* inside on exactly on the boundary */
int pointInAABB(struct vec2 p, struct aabb box){
	double x = p.x;
	double y = p.y;
	return box.x0 <= x && x <= box.x1 && box.y0 <= y && y <= box.y1;
}

int aabbApart(struct aabb A, struct aabb B){
	return A.x1 < B.x0 || B.x1 < A.x0 || A.y1 < B.y0 || B.y1 < A.y0;
}


/* https://bryceboe.com/2006/10/23/line-segment-intersection-algorithm/ */
/* note it doesn't handle parallel segments */
int segmentsCross(vec2 a, vec2 b, vec2 c, vec2 d) {
	#define CCW(A,B,C) ((C.y-A.y)*(B.x-A.x) > (B.y-A.y)*(C.x-A.x))
	return CCW(a,c,d) != CCW(b,c,d) && CCW(a,b,c) != CCW(a,b,d);
}

int segmentsIntersection(struct vec2 p0, struct vec2 p1, struct vec2 q0, struct vec2 q1, vec2 *out){

	if(!segmentsCross(p0,p1,q0,q1)) return 0;

	double abc1[3];
	double abc2[3];

	abcForm(p0, p1, abc1);
	abcForm(q0, q1, abc2);
	int bad = solve2x2(abc1, abc2, out);

	return bad ? 0 : 1;

}
