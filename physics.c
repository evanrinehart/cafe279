#include <stdio.h>
#include <symbols.h>
#include <linear.h>
#include <doodad.h>
#include <physics.h>

#include <math.h>

#define PI M_PI

struct Object objects[1024];
struct Object *objects_ptr = objects;
struct Object *objects_end = objects + 1024;

struct ClipResult {
	double fraction;
	vec2 hitpoint;
};

const struct ClipResult NO_CLIP = { .fraction = INFINITY };

struct ClipResult clipResults[64];
struct ClipResult *clipResults_ptr = clipResults;
struct ClipResult *clipResults_end = clipResults + 64;

void addClipResult(struct ClipResult cr){
	*clipResults_ptr++ = cr;
}

void clearClipResults(){
	clipResults_ptr = clipResults;
}

struct ClipResult minimumClipResult(){
	struct ClipResult cr = NO_CLIP;
	for(struct ClipResult *ptr = clipResults; ptr < clipResults_ptr; ptr++){
		if(ptr->fraction < cr.fraction) cr = *ptr;
	}
	return cr;
}

vec2 a = {0, -36};

struct ClipResult endclip(vec2 c, vec2 cjump, double r, vec2 endp){
	vec2 p = sub(endp, c);
	vec2 jump = neg(cjump);
	// Assuming p is outside circle centered at origin with radius r
	// how far to p + jump does it get before it intersects the circle
	double A = dot(jump, jump);
	double B = 2 * dot(p, jump);
	double C = dot(p, p) - r * r;
	double t;
	int result = quadraticFormula(A,B,C,&t);
	if(result < 0) return NO_CLIP;
	else if(t < 0) return NO_CLIP;
	else if(t < 1.0) {
		struct ClipResult cr = {t, endp};
		return cr;
	}
	else return NO_CLIP;
}

struct ClipResult clip(vec2 a, vec2 jump, double r, vec2 wallA, vec2 wallB){
	// Assuming: disc centered at `a' radius `r' is outside wall
	// and new position `b' is sufficiently far from `a'.
	// Return: fraction of path allowed before hitting the wall.

	// line P Q goes right, wall is below
	// follow P to Q, up out of screen, wall is on your right

	// seg x path > 0, moving away from wall
	// seg x path < 0, moving into wall

	vec2 wall = sub(wallB, wallA); // q - p

//printf("cross = %lf\n", cross(wall,jump));
	if(cross(wall, jump) >= 0) return NO_CLIP; // moving away from wall

	vec2 jump_into = rejection(jump, wall);
	vec2 b_into = add(a, jump_into);
	vec2 correction = scale(r, normalize(jump_into));

	vec2 aprime = add(a, correction);
	vec2 bprime = add(b_into, correction);

	vec2 hitPoint;
//printf("aprime = %lf %lf\n", aprime.x, aprime.y);
//printf("bprime = %lf %lf\n", bprime.x, bprime.y);
//printf("wallA = %lf %lf\n", wallA.x, wallA.y);
//printf("wallB = %lf %lf\n", wallB.x, wallB.y);
//printf("SI = %d\n", segmentsIntersection(aprime, bprime, wallA, wallB, &hitPoint));
	if(!segmentsIntersection(aprime, bprime, wallA, wallB, &hitPoint)) return NO_CLIP;

	double fraction = normSquared(sub(hitPoint, aprime)) / normSquared(sub(bprime, aprime));

	struct ClipResult cr = {fraction, hitPoint};
	return cr;
}

double perpendicularDistanceSigned(vec2 p, vec2 a, vec2 b){
	// shift problem so a is at the origin
	vec2 wall = sub(b, a);
	vec2 q    = sub(p, a);
	// the standoff vector is just the rejection
	vec2 standoff = rejection(q, wall);
	// negative sign if p is inside wall
	double D = norm(standoff);
	if(cross(wall, standoff) >= 0) return D;
	else return -D;
}


void physObject(struct Object *obj){
	vec2 jump = scale(1/60.0, obj->vel);
	vec2 wallA = {-16 * 3.999, 8};
	vec2 wallB = {16 * 4.001, 8};

	clearClipResults();

	struct ClipResult cr;
	cr = clip(obj->pos, jump, 4, wallA, wallB);  if(cr.fraction < 1.0) addClipResult(cr);
//printf("test1 = %lf\n", cr.fraction);
	cr = endclip(obj->pos, jump, 4, wallA); if(cr.fraction < 1.0) addClipResult(cr);
//printf("test2 = %lf\n", cr.fraction);
	cr = endclip(obj->pos, jump, 4, wallB); if(cr.fraction < 1.0) addClipResult(cr);
//printf("test3 = %lf\n", cr.fraction);

	cr = minimumClipResult();
//printf("fraction = %lf\n", cr.fraction);
	if(cr.fraction < 1.0){
		vec2 newpos = add(obj->pos, scale(cr.fraction, jump));
//printf("newpos = %lf %lf\n", newpos.x, newpos.y);
		vec2 hit = cr.hitpoint;
//printf("hit = %lf %lf\n", hit.x, hit.y);
		vec2 rad = sub(newpos, hit);
//printf("rad = %lf %lf\n", rad.x, rad.y);
		vec2 surf = rotate(rad, -PI/2);
//printf("surf = %lf %lf\n", surf.x, surf.y);
		obj->pos = newpos;
		printf("speed before = %lf\n", norm(obj->vel));
		printf("v before = %lf %lf\n", obj->vel.x, obj->vel.y);
		obj->vel = reflection(obj->vel, surf);
		printf("speed after = %lf\n", norm(obj->vel));
		printf("v after = %lf %lf\n", obj->vel.x, obj->vel.y);
	}
	else{
		obj->pos = add(obj->pos, jump);
		obj->vel = add(obj->vel, scale(1/60.0, a));
	}

}

void addObject(){
	struct Object *obj = objects_ptr++;
	obj->pos = vec2(-16 * 4, 64);
	obj->vel = vec2(0,0);
}

/* run physics for 1 step */
void physics(){

	for(struct Object *obj = objects; obj < objects_ptr; obj++){
		physObject(obj);
	}

	for(struct Doodad *d = doodads; d < doodad_ptr; d++){
		updateDoodad(d);
	}

}
