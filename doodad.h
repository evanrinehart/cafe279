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

void clickDoodad(struct Doodad *d);
struct Doodad * findDoodad(vec2 p);

#define MAX_DOODADS 1024
extern struct Doodad doodads[MAX_DOODADS];
extern struct Doodad *doodad_ptr;
extern struct Doodad *doodad_ptr_end;
