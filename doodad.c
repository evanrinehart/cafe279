#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <linear.h>
#include <symbols.h>
#include <doodad.h>

#define MAX_DOODADS 1024
struct Doodad doodads[MAX_DOODADS];
struct Doodad *doodad_ptr = doodads;
struct Doodad *doodad_ptr_end = doodads + MAX_DOODADS;

void addDoodad(struct Doodad d){
	*doodad_ptr++ = d;
}

void deleteDoodad(struct Doodad *d){
	*d = *--doodad_ptr;
}

void printDoodad(struct Doodad d){
	printf(
		"Doodad(serial_no=%d pos=(%lf,%lf) label=\"%s\" symbol=%s)\n",
		d.serial_no,
		d.pos.x,
		d.pos.y,
		d.label,
		stringifySymbol(d.symbol)
	);
}

struct Doodad randomDoodad(){
	char names[][64] = {"HELLO", "WORLD", "SORTED", "SYNTAX", "VALUE", "SPACE", "COLD", "ACCOUNT"};
	struct Doodad d;
	d.serial_no = rand() % 1000;
	d.pos.x = 10 * ((1.0 * rand() / INT_MAX) - 0.5);
	d.pos.y = 10 * ((1.0 * rand() / INT_MAX) - 0.5);
	strcpy(d.label, names[rand() % 8]);
	d.symbol = rand() % 8;
	return d;
}

/*
void main(){
	printDoodad(randomDoodad());
	printDoodad(randomDoodad());
	printDoodad(randomDoodad());
	printDoodad(randomDoodad());

	addDoodad(randomDoodad());
	addDoodad(randomDoodad());
	addDoodad(randomDoodad());
	addDoodad(randomDoodad());

	for(struct Doodad *ptr = doodads; ptr < doodad_ptr; ptr++){
		printDoodad(*ptr);
	}

	for(struct Doodad *ptr = doodads; ptr < doodad_ptr; ptr++){
		printDoodad(*ptr);
	}
}

*/
