struct Doodad {
	int serial_no;
	vec2 pos;
	char label[64];
	enum Symbol symbol;
};

void addDoodad(struct Doodad *d);
void deleteDoodad(struct Doodad *d);
void printDoodad(struct Doodad d);
struct Doodad randomDoodad();
