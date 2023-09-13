#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <linear.h>

#include <doodad.h>

#define PI M_PI

struct Doodad doodads[MAX_DOODADS];
struct Doodad *doodads_ptr = doodads;
struct Doodad *doodads_end = doodads + MAX_DOODADS;

void addDoodad(struct Doodad *d){
	if(doodads_ptr < doodads_end) *doodads_ptr++ = *d;
}

void deleteDoodad(struct Doodad *d){
	if(doodads_ptr > doodads) *d = *--doodads_ptr;
}

void printDoodad(struct Doodad *d){
	printf(
		"Doodad(serial_no=%d pos=(%lf,%lf) label=\"%s\" symbol=%s)\n",
		//"(%d, %lf,%lf, \"%s\", \"%s\")\n",
		d->serial_no,
		d->pos.x,
		d->pos.y,
		d->label,
		stringifySymbol(d->symbol)
	);
}

struct Doodad randomDoodad(){
	char names[][64] = {"HELLO", "WORLD", "SORTED", "ELEMENT", "VALUE", "SPACE", "COLD", "ACCOUNT"};
	struct Doodad d;
	d.serial_no = rand() % 1000;
	d.pos.x = 16 * 10 * ((1.0 * rand() / INT_MAX) - 0.5);
	d.pos.y = 16 * 10 * ((1.0 * rand() / INT_MAX) - 0.5);
	strcpy(d.label, names[rand() % 8]);
	d.symbol = rand() % 8;
	return d;
}


void clickDoodad(struct Doodad *d){
	int L = strlen(d->label); if(L == 0) return;
	int c = d->label[0];
	for(int i=0; i < L; i++){ d->label[i] = d->label[i+1]; }
	d->label[L-1] = c;
	printDoodad(d);
}

struct Doodad * findDoodad(vec2 mouse){
	for(struct Doodad *ptr = doodads; ptr < doodads_ptr; ptr++){
		vec2 p = ptr->pos;
		struct aabb box = {p.x - 5, p.x + 5, p.y - 5, p.y + 5};
		if(pointInAABB(mouse, box)){
			return ptr;
		}
	}
	return NULL;
}


void updateDoodad(struct Doodad *doodad){
	vec2 p = doodad->pos;
	double r = norm(p);
	double angle = atan2(p.y, p.x);
	doodad->pos = rcis(r, angle + 0.05 * 0.01666 * 2 * PI);
}

const char * stringifySymbol(enum Symbol sym){
	switch(sym){
		case NEPTUNE_SYMBOL: return "NEPTUNE_SYMBOL";
		case MOON_SYMBOL:    return "MOON_SYMBOL";
		case NULL_SYMBOL:    return "NULL_SYMBOL";
		case SUN_SYMBOL:     return "SUN_SYMBOL";
		case SATURN_SYMBOL:  return "SATURN_SYMBOL";
		case GALAXY_SYMBOL:  return "GALAXY_SYMBOL";
		default:             return "UNKNOWN_SYMBOL";
	}
}

enum Symbol parseSymbol(const char * str){
	if(strcmp(str, "NEPTUNE_SYMBOL") == 0) return NEPTUNE_SYMBOL;
	if(strcmp(str, "MOON_SYMBOL") == 0)    return MOON_SYMBOL;
	if(strcmp(str, "NULL_SYMBOL") == 0)    return NULL_SYMBOL;
	if(strcmp(str, "SUN_SYMBOL") == 0)     return SUN_SYMBOL;
	if(strcmp(str, "SATURN_SYMBOL") == 0)  return SATURN_SYMBOL;
	if(strcmp(str, "GALAXY_SYMBOL") == 0)  return GALAXY_SYMBOL;
	return UNKNOWN_SYMBOL;
}
