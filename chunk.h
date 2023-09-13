#define CHUNK_SIZE 256

/*
A note on large square arrays and indices.

The index i picks row.
The index j picks element from a row.

Also the rows are vertical. So the center of cell i,j is (i * CELL, j * CELL)

*/

struct Chunk {
	int block[CHUNK_SIZE][CHUNK_SIZE]; // walls, floors, 0 = no block
	int therm[CHUNK_SIZE][CHUNK_SIZE]; // thermal energy units
	int wind[CHUNK_SIZE][CHUNK_SIZE]; // direction code
	int room[CHUNK_SIZE][CHUNK_SIZE]; // 0 = inside block (no room), 1 = outside, 2+ are proper rooms

	char atmo_edges_h[CHUNK_SIZE][CHUNK_SIZE]; // atmo edges block airflow and hold atmosphere, can be 0 1 or 2
	char atmo_edges_v[CHUNK_SIZE][CHUNK_SIZE];
	char room_edges_h[CHUNK_SIZE][CHUNK_SIZE]; // room edges bound a room holding a unique atmosphere, 0 or 1
	char room_edges_v[CHUNK_SIZE][CHUNK_SIZE];

	int outsideAirPerCell;
};

struct CellWindow {
	int imin;
	int imax;
	int jmin;
	int jmax;
};

struct Room {
	int id;
	int air;
	int volume;
};

#define MAX_ROOMS 1000
extern struct Room rooms[MAX_ROOMS];
extern struct Room *rooms_ptr;
extern struct Room *rooms_end;

extern struct Chunk chunk;

#define CELL 16.0                                // the size of a cell in (logical) pixels
#define REDUCE(x) (floor(((x) + CELL/2) / CELL)) // cell index for world coordinate
#define EXPAND(i) (CELL*(i))                     // world coordinate for center of cell at index

void addAtmoBlockEdges(int i, int j);
void subAtmoBlockEdges(int i, int j);
int roomExists(int rid, int *outi, int *outj);
void refreshRoomEdges(int i, int j);

void showRooms(void);
void initializeRooms(void);
void finishChunkLoading(void);

struct CellWindow discFootprint(vec2 c, double r);
struct CellWindow unionCellWindow(struct CellWindow w1, struct CellWindow w2);
void cellCorners(int i, int j, vec2 corners[4]);

void putBlock(int i, int j, int type);
void eraseBlock(int i, int j);

int isOutside(int i, int j);
int measureRoom(int i, int j);
int measureCavity(int i, int j);

void addRoom(int id, int air, int volume);
struct Room * roomById(int id);
