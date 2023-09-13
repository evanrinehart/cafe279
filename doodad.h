enum Symbol {
	NEPTUNE_SYMBOL,
	MOON_SYMBOL,
	SUN_SYMBOL,
	SATURN_SYMBOL,
	GALAXY_SYMBOL,
	NULL_SYMBOL,
	UNKNOWN_SYMBOL
};

struct Doodad {
	int serial_no;
	vec2 pos;
	char label[64];
	enum Symbol symbol;
};

void addDoodad(struct Doodad *d);
void deleteDoodad(struct Doodad *d);
void printDoodad(struct Doodad *d);
struct Doodad randomDoodad();

const char * stringifySymbol(enum Symbol sym);
enum Symbol parseSymbol(const char * str);

void clickDoodad(struct Doodad *d);
struct Doodad * findDoodad(vec2 p);

void updateDoodad(struct Doodad *doodad);

#define MAX_DOODADS 1024
extern struct Doodad doodads[MAX_DOODADS];
extern struct Doodad *doodads_ptr;
extern struct Doodad *doodads_end;
