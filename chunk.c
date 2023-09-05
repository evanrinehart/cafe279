#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include <chunk.h>

#include <floodfill.h>

struct Chunk chunk;

struct Room rooms[MAX_ROOMS];
int room_counter = 2;

// rooms
#define LOOP256(I,J,X) \
	for(int J = 0; J < 256; J++){ \
		for(int I = 0; I < 256; I++){ \
			do{ X; } while(0); \
		} \
	}

void refreshRoomEdges(int i, int j){
	int r = chunk.room[i][j];
	int r0 = i < 255 ? chunk.room[i+1][j] : 0;
	int r1 = j > 0   ? chunk.room[i][j-1] : 0;
	int r2 = i > 0   ? chunk.room[i-1][j] : 0;
	int r3 = j < 255 ? chunk.room[i][j+1] : 0;

	if(i < 255) chunk.room_edges_v[i+1][j] = r != r0;
	chunk.room_edges_h[i][j] =   r != r1;
	chunk.room_edges_v[i][j] =   r != r2;
	if(j < 255) chunk.room_edges_h[i][j+1] = r != r3;
}

void refreshAtmoEdge(int i, int j){
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


int outside_test_visitor(void* u, int i, int j){
	return i == 0 || j == 0 || i == 255 || j == 255;
}

int isOutside(int i, int j){
	return flood(i, j, chunk.atmo_edges_h, chunk.atmo_edges_v, NULL, outside_test_visitor);
}

int counting_visitor(void* u, int i, int j){
	int *c = u;
	(*c)++;
	return 0;
}

int measureVolume(int i, int j){
	int counter = 0;
	flood(i, j, chunk.room_edges_h, chunk.room_edges_v, &counter, counting_visitor);
	return counter;
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

int roomExists(int r){
	LOOP256(i,j, if(chunk.room[i][j] == r) return 1;)
	return 0;
}

void showRooms(){
	printf("%10s %10s %10s %10s\n", "id", "air", "volume", "pressure");
	for(int i = 1; i < room_counter; i++){
		struct Room * ptr = &rooms[i];
		if(i == 1){
			printf("%10d %10d %10d %10.4f\n", ptr->id, ptr->air, ptr->volume, (float)chunk.outsideAirPerCell);
		}
		else if(i > 1 && roomExists(i)){
			printf("%10d %10d %10d %10.4f\n", ptr->id, ptr->air, ptr->volume, 1.0*ptr->air/ptr->volume);
		}
	}
}

void findRoomCell(int r, int *ri, int *rj){
	LOOP256(i,j, if(chunk.room[i][j] == r){ *ri = i; *rj = j; })
}

void updateRoomVolume(int i, int j){
	int r = chunk.room[i][j];
	if(r > 0) rooms[r].volume = measureVolume(i, j);
}

struct Room * createNewRoom(int i, int j, int air, int volume){
	if(room_counter == MAX_ROOMS) return NULL;

printf("DING new room (%d) created at %d %d\n", room_counter, i, j);

	int k = room_counter++;

	struct Room *ptr = &rooms[k];
	ptr->id = k;
	ptr->volume = volume;
	ptr->air = air;

	//floodfill(i, j, k, chunk.room);
	chunk.room[i][j] = k;

	return ptr;
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


void initializeRooms(){

	chunk.outsideAirPerCell = 250;

	rooms[0].id = 0;
	rooms[0].air = 0;
	rooms[0].volume = 65535;
	rooms[1].id = 1;
	rooms[1].air = 0;
	rooms[1].volume = 65535;

	// legacy load from db
	LOOP256(i, j,
		if(chunk.block[i][j]){
			//chunk.block[i][j] = 1;
			chunk.room[i][j] = 0;
		}
		else{
			//chunk.block[i][j] = 0;
			chunk.room[i][j] = 1;
		}
	);

	LOOP256(i, j, refreshRoomEdges(i,j));
	LOOP256(i, j, refreshAtmoEdge(i,j));

/*
	LOOP256(i, j,
		paper[i][j]      = chunk.block[i][j] ? -127 : 0;
		badoutside[i][j] = paper[i][j];
		chunk.room[i][j] = chunk.block[i][j] ? 0 : -1;
	)

	LOOP256(i, j,
		if(i==0 || j==0 || i==255 || j==255){
			if(chunk.room[i][j] == -1){
				floodfill(i, j, 1, chunk.room);
			}
		}
	)

	LOOP256(i, j,
		if(chunk.room[i][j] == -1){
			int volume = floodvolume(i,j,chunk.room);
			createNewRoom(i, j, 0, volume); // eventually load air from save
		}
	)
*/
}


int canGoEast(int i, int j) { return chunk.atmo_edges_v[i+1][j] == 0; }
int canGoSouth(int i, int j){ return chunk.atmo_edges_h[i][j] == 0; }
int canGoWest(int i, int j) { return chunk.atmo_edges_v[i][j] == 0; }
int canGoNorth(int i, int j){ return chunk.atmo_edges_h[i][j+1] == 0; }

int accessibleAdjacentRoom(int i,int j){
	if(canGoEast(i,j)) return chunk.room[i+1][j];
	if(canGoSouth(i,j)) return chunk.room[i][j-1];
	if(canGoWest(i,j)) return chunk.room[i-1][j];
	if(canGoNorth(i,j)) return chunk.room[i][j+1];
	return 0;
}

void addBlock(int i, int j){
	chunk.block[i][j] = 1;
	chunk.room[i][j] = 0;
	addAtmoBlockEdges(i, j);
	//newPlacedPiece(i-128, j-128, 0, 250);
	refreshRoomEdges(i, j);
}

void deleteBlock(int i, int j){
	chunk.block[i][j] = 0;
	subAtmoBlockEdges(i, j);
	//deleteTileAt(i-128,j-128); // legacy

	int r = accessibleAdjacentRoom(i,j);

	if(r){
		chunk.room[i][j] = r;
	}
	else{
		createNewRoom(i,j,0,1);
	}
		
	refreshRoomEdges(i, j);
}

