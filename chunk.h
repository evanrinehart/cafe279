#define CHUNK_SIZE 256

struct Chunk {
	int block[CHUNK_SIZE][CHUNK_SIZE]; // walls, floors, 0 = no block
	int therm[CHUNK_SIZE][CHUNK_SIZE]; // thermal energy units
	int wind[CHUNK_SIZE][CHUNK_SIZE]; // direction code
	int room[CHUNK_SIZE][CHUNK_SIZE]; // 0 = no room i.e. outside

	char atmo_edges_h[CHUNK_SIZE][CHUNK_SIZE]; // atmo edges block airflow and hold atmosphere, can be 0 1 or 2
	char atmo_edges_v[CHUNK_SIZE][CHUNK_SIZE];
	char room_edges_h[CHUNK_SIZE][CHUNK_SIZE]; // room edges bound a room holding a unique atmosphere, 0 or 1
	char room_edges_v[CHUNK_SIZE][CHUNK_SIZE];

	int outsideAirPerCell;
};

extern struct Chunk chunk;

struct Room {
	int id;
	int air;
	int volume;
};

#define MAX_ROOMS 1000
extern struct Room rooms[MAX_ROOMS];
extern int room_counter;



void addAtmoBlockEdges(int i, int j);
void subAtmoBlockEdges(int i, int j);
int roomExists(int r);
void refreshRoomEdges(int i, int j);
struct Room * createNewRoom(int i, int j, int air, int volume);
void initializeRooms();
int measureVolume(int i, int j);
int isOutside(int i, int j);
