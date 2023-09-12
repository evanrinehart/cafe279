#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include <linear.h>
#include <chunk.h>

#include <floodfill.h>

struct Chunk chunk;

struct Room rooms[MAX_ROOMS];
struct Room *rooms_ptr = rooms;
struct Room *rooms_end = rooms + MAX_ROOMS;

int highest_room_id = 1;

#define LOOP256(I,J) \
	for(int I = 0; I < 256; I++) \
		for(int J = 0; J < 256; J++)

void refreshRoomEdges(int i, int j);
void refreshAtmoEdges(int i, int j);
void addRoom(int id, int air, int volume);
void deleteRoom(struct Room *room);
int measureCavity(int i, int j);
int measureRoom(int i, int j);

int floodAtmosphere(int i, int j, void* u, int (*visit)(void*, int, int)){
	return flood(i, j, chunk.atmo_edges_h, chunk.atmo_edges_v, u, visit);
}

int floodRoom(int i, int j, void* u, int (*visit)(void*, int, int)){
	return flood(i, j, chunk.room_edges_h, chunk.room_edges_v, u, visit);
}

// right after generating map, initialize the rooms
// only blocks will be defined
void initializeRooms(){

	chunk.outsideAirPerCell = 250;

	rooms[0].id = 0; // room 0 is not a room, this is a placeholder
	rooms[0].air = 0;
	rooms[0].volume = 65535;
	rooms[1].id = 1; // room 1 is special, outdoors
	rooms[1].air = 0;
	rooms[1].volume = 65535;

	// initialize open spaces to room -1 to be filled in later
	LOOP256(i, j) chunk.room[i][j] = chunk.block[i][j] ? 0 : -1;

	// establish atmo edges so floodfill works.
	LOOP256(i, j) refreshAtmoEdges(i,j);

	// if -1 is found on the border flood it with 1 (outside)
	int mark_as_outside(void* unused, int i, int j){ chunk.room[i][j] = 1; return 0; }

	LOOP256(i, j) {
		if ((i==0 || j==0 || i==255 || j==255) && chunk.room[i][j] == -1) {
			floodAtmosphere(i, j, NULL, mark_as_outside);
		}
	}

	int mark_as_room_id(void* unused, int i, int j){ chunk.room[i][j] = highest_room_id; return 0; }

	// if -1 is found now, it's inside, make it a room
	LOOP256(i, j) {
		if(chunk.room[i][j] == -1){
			highest_room_id++;
			int volume = measureCavity(i,j);
			floodAtmosphere(i, j, NULL, mark_as_room_id);
			addRoom(highest_room_id, 0, volume);
		}
	}

	// all rooms identified, establish room boundaries
	LOOP256(i, j) refreshRoomEdges(i,j);
}

void initializeOutdoorsOnly(){

	chunk.outsideAirPerCell = 250;

	rooms[0].id = 0; // room 0 is not a room, this is a placeholder
	rooms[0].air = 0;
	rooms[0].volume = 65535;
	rooms[1].id = 1; // room 1 is special, outdoors
	rooms[1].air = 0;
	rooms[1].volume = 65535;

	// initialize open spaces to room -1 to be filled in later
	LOOP256(i, j) {
		if(chunk.room[i][j] > 1) continue;
		chunk.room[i][j] = chunk.block[i][j] ? 0 : 1;
	}

	// establish atmo edges so floodfill works.
	LOOP256(i, j) refreshAtmoEdges(i,j);

	// all rooms identified, establish room boundaries
	LOOP256(i, j) refreshRoomEdges(i,j);
}

void refreshRoomEdges(int i, int j){
	int r = chunk.room[i][j];
	int r0 = i < 255 ? chunk.room[i+1][j] : 0;
	int r1 = j > 0   ? chunk.room[i][j-1] : 0;
	int r2 = i > 0   ? chunk.room[i-1][j] : 0;
	int r3 = j < 255 ? chunk.room[i][j+1] : 0;

	if(i < 255) chunk.room_edges_v[i+1][j] = r != r0;
	chunk.room_edges_h[i][j] = r != r1;
	chunk.room_edges_v[i][j] = r != r2;
	if(j < 255) chunk.room_edges_h[i][j+1] = r != r3;
}

void refreshAtmoEdges(int i, int j){
	int here = !!chunk.block[i][j];

	if(i > 0){
		int west = !!chunk.block[i-1][j];
		chunk.atmo_edges_v[i][j] = here + west;
	}
	
	if(j > 0){
		int south = !!chunk.block[i][j-1];
		chunk.atmo_edges_h[i][j] = here + south;
	}
}

// cell "is outside" if it can reach the world boundary
int outside_test_visitor(void* u, int i, int j){ return i == 0 || j == 0 || i == 255 || j == 255; }
int isOutside(int i, int j){ return floodAtmosphere(i, j, NULL, outside_test_visitor); }

// Volume measuring, respect room boundary or not
int volume_counter;

int counting_visitor(void* u, int i, int j){ volume_counter++; return 0; }

int measureCavity(int i, int j){
	volume_counter = 0;
	floodAtmosphere(i, j, NULL, counting_visitor);
	return volume_counter;
}

int measureRoom(int i, int j){
	volume_counter = 0;
	floodRoom(i, j, NULL, counting_visitor);
	return volume_counter;
}


void addAtmoBlockEdges(int i, int j){
	if(i==0 || j==0 || i==255 || j==255) goto alert1;
	chunk.atmo_edges_v[i][j]++;
	chunk.atmo_edges_v[i+1][j]++;
	chunk.atmo_edges_h[i][j]++;
	chunk.atmo_edges_h[i][j+1]++;
	return;

alert1:
	printf("addAtmoBlockEdges -- editing edge of map\n");
	return;
}

void subAtmoBlockEdges(int i, int j){

	if(i==0 || j==0 || i==255 || j==255) goto alert1;
	if(chunk.atmo_edges_v[i][j] == 0)   goto alert2;
	if(chunk.atmo_edges_v[i+1][j] == 0) goto alert2;
	if(chunk.atmo_edges_h[i][j] == 0)   goto alert2;
	if(chunk.atmo_edges_h[i][j+1] == 0) goto alert2;
	chunk.atmo_edges_v[i][j]--;
	chunk.atmo_edges_v[i+1][j]--;
	chunk.atmo_edges_h[i][j]--;
	chunk.atmo_edges_h[i][j+1]--;

	return;

alert1:
	printf("subAtmoBlockEdges -- editing edge of map\n");
	return;

alert2:
	printf("subAtmoBlockEdges -- subtracting atmo edges where there are none\n");
	return;
}

double computeRoomPressure(int r, int volumeAdjust){
	if(r == 1) return chunk.outsideAirPerCell;
	if(r > 1) return (double)rooms[r].air / (rooms[r].volume + volumeAdjust);
	return 0;
}

void showRooms(){
	int i = 0;
	printf("%10s %10s %10s %12s %10s\n", "id", "air", "volume", "pressure", "note");
	printf("%10d %10s %10s %12s\n", 0, "-", "-", "-");
	printf("%10d %10s %10s %12.4f %10s\n", 1, "a lot", "-", 1.0*chunk.outsideAirPerCell, "outside");

	for(struct Room *ptr = rooms; ptr < rooms_ptr; ptr++, i++){
		printf("%10d %10d %10d %12.4f\n", ptr->id, ptr->air, ptr->volume, 1.0*ptr->air/ptr->volume);
	}
}

void findRoomCell(int r, int *ri, int *rj){
	LOOP256(i,j) {if(chunk.room[i][j] == r){ *ri = i; *rj = j; }};
}


int roomsCanMerge(int r1, int r2){
	struct Room *ptr1 = &rooms[r1];
	struct Room *ptr2 = &rooms[r2];

	double a1 = ptr1->air;
	double a2 = ptr2->air;
	double v1 = ptr1->volume;
	double v2 = ptr2->volume;

	double airDiff = (a2*v1 - a1*v2) / (v1 + v2);

	return fabs(airDiff) < 1;
}

/*
void mergeTwoRooms(int r1, int r2){
	if(r1 == r2) return;
	if(r1 < 1 || r2 < 1) return;

	struct Room *ptr1 = &db_rooms[r1];
	struct Room *ptr2 = &db_rooms[r2];

	
	double air = ptr1->air + ptr2->air;
	double volume = ptr1->volume + ptr2->volume;

	if(r1 < r2){ // sacrifice the elder
		ptr1->air = air;
		ptr1->volume = volume;
		int ri=0, rj=0;
		findRoomCell(r2, &ri, &rj);
		floodfill(ri, rj, r1, chunk.room);
	}
	else{
		ptr2->air = air;
		ptr2->volume = volume;
		int ri=0, rj=0;
		findRoomCell(r1, &ri, &rj);
		floodfill(ri, rj, r2, chunk.room);
	}
}
*/



/* chunk routines 2.0 */

void addBlock(int i, int j){
	chunk.block[i][j] = 1;
	chunk.room[i][j] = 0;
	addAtmoBlockEdges(i, j);
	//newPlacedPiece(i-128, j-128, 0, 250);
	refreshRoomEdges(i, j);
}

#define min(A,B) (A < B ? A : B)
#define max(A,B) (A > B ? A : B)

struct CellWindow discFootprint(vec2 c, double r){

	int i1 = REDUCE(c.x - r);
	int i3 = REDUCE(c.x + r);
	int j1 = REDUCE(c.y - r);
	int j3 = REDUCE(c.y + r);

	struct CellWindow w = {
		.imin = min(i1,i3),
		.imax = max(i1,i3),
		.jmin = min(j1,j3),
		.jmax = max(j1,j3)
	};

	return w;
}

struct Quad {
	vec2 a;
	vec2 b;
	vec2 c;
	vec2 d;
};

void cellCorners(int i, int j, vec2 corners[4]){
	double half = CELL / 2;
	vec2 center = {EXPAND(i), EXPAND(j)}; // "i is x, rows go vertical"
	corners[0].x = center.x + half;
	corners[0].y = center.y + half;
	corners[1].x = center.x + half;
	corners[1].y = center.y - half;
	corners[2].x = center.x - half;
	corners[2].y = center.y - half;
	corners[3].x = center.x - half;
	corners[3].y = center.y + half;
}

int neighboringRoom(int i, int j){
	int r;
	r = chunk.room[i+1][j]; if(r) return r;
	r = chunk.room[i][j-1]; if(r) return r;
	r = chunk.room[i-1][j]; if(r) return r;
	r = chunk.room[i][j+1]; if(r) return r;
	return 0;
}

void putBlock(int i, int j, int type){
	if(chunk.block[i][j]) return;

	chunk.block[i][j] = type;
	chunk.room[i][j] = 0;

	refreshRoomEdges(i,j);

	chunk.atmo_edges_h[i][j]++;
	chunk.atmo_edges_v[i][j]++;
	chunk.atmo_edges_h[i][j+1]++;
	chunk.atmo_edges_v[i+1][j]++;
}

void eraseBlock(int i, int j){
	if(chunk.block[i][j] == 0) return;

	chunk.block[i][j] = 0;
	subAtmoBlockEdges(i, j);

	int r = neighboringRoom(i,j);
	if(r) chunk.room[i][j] = r;
	else {
		highest_room_id++;
		chunk.room[i][j] = highest_room_id;
		addRoom(highest_room_id, 0, 1);
	}

	refreshRoomEdges(i,j);
}

void addRoom(int id, int air, int volume){
	if(id > highest_room_id) highest_room_id = id;
	if(rooms_ptr == rooms_end) return;
	struct Room room = { .id = id, .air = air, .volume = volume };
	*rooms_ptr++ = room;
	printf("DING new room created id=%d air=%d volume=%d\n", id, air, volume);
}

void deleteRoom(struct Room *room){
	rooms_ptr--;
	*room = *rooms_ptr;
}

struct Room * roomById(int id){
	for(struct Room *ptr = rooms; ptr < rooms_end; ptr++){
		if(ptr->id == id) return ptr;
	}
	return NULL;
}

int roomExists(int r){
	LOOP256(i,j) { if(chunk.room[i][j] == r) return 1; }
	return 0;
}
