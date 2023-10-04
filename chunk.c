#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include <linear.h>
#include <chunk.h>

#include <floodfill.h>

#include <bsod.h>

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

static int mark_as_outside(void* u, int i, int j){ chunk.room[i][j] = 1; return 0; }
static int mark_as_room_id(void* u, int i, int j){ chunk.room[i][j] = highest_room_id; return 0; }

// right after generating map, initialize the rooms
// only blocks will be defined
void initializeRooms(){

	return; // this is disabled for now, just load the rooms from workspace file

	chunk.outsideAirPerCell = 250;

	highest_room_id = 1;

	// initialize open spaces to room -1 to be filled in later
	LOOP256(i, j) chunk.room[i][j] = chunk.block[i][j] ? 0 : -1;

	// establish atmo edges so floodfill works.
	LOOP256(i, j) refreshAtmoEdges(i,j);

	// if -1 is found on the border flood it with 1 (outside)

	LOOP256(i, j) {
		if ((i==0 || j==0 || i==255 || j==255) && chunk.room[i][j] == -1) {
			floodAtmosphere(i, j, NULL, mark_as_outside);
		}
	}


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

void finishChunkLoading(){

	// loader has produced blocks, rooms
	// still missing the outdoor cells, room volumes

	chunk.outsideAirPerCell = 250;

	LOOP256(i, j) {
		// zero might mean it's a wall, or outdoors which was not saved or loaded
		if(chunk.room[i][j] == 0) chunk.room[i][j] = chunk.block[i][j] ? 0 : 1;
	}

	// establish atmo edges so floodfill works.
	LOOP256(i, j) refreshAtmoEdges(i,j);

	// all rooms identified, establish room boundaries
	LOOP256(i, j) refreshRoomEdges(i,j);

	for(struct Room *room = rooms; room < rooms_ptr; room++){
		int i, j;
		if (roomExists(room->id, &i, &j)) room->volume = measureRoom(i,j);
	}
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

int counting_visitor(void* u, int i, int j){
	volume_counter++;
	return 0;
}

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

struct CellWindow unionCellWindow(struct CellWindow w1, struct CellWindow w2){
	struct CellWindow w3 = {
		min(w1.imin, w2.imin),
		max(w1.imax, w2.imax),
		min(w1.jmin, w2.jmin),
		max(w1.jmax, w2.jmax)
	};
	return w3;
}

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

void shrinkRoom(int rid){
	if(rid <= 1) return;

	struct Room *room = roomById(rid);
	if(room == NULL) bsod("sR room not found");

	if(room->volume == 1) deleteRoom(room);
	else room->volume--;
}

void growRoom(int rid){
	if(rid <= 1) return;

	struct Room *room = roomById(rid);
	if(room == NULL) bsod("gR room not found");

	room->volume++;
}

void putBlock(int i, int j, int type){
	if(chunk.block[i][j]) return;

	int rid = chunk.room[i][j];

	shrinkRoom(rid);

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

	int rid = neighboringRoom(i,j);
	if(rid) {
		chunk.room[i][j] = rid;
		growRoom(rid);
	}
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

int roomExists(int rid, int *outi, int *outj){
	LOOP256(i,j) {
		if(chunk.room[i][j] == rid) {
			if(outi) *outi = i;
			if(outj) *outj = j;
			return 1;
		}
	}
	return 0;
}


/*
void rotateRaw(enum Rot rot, int x, int y, int *xout, int *yout) {
	int top = 1 << 20;
	switch (rot) {
	case THREE_HOURS:
		*xout = y;
		*yout = top - x;
		break;
	case SIX_HOURS:
		*xout = top - x;
		*yout = top - y;
		break;
	case NINE_HOURS:
		*xout = top - y;
		*yout = x;
		break;
	case ZERO_HOURS:
		*xout = x;
		*yout = y;
		break;
	}
}

int rotBlock(int block) {
	switch (block) {
	case 0:   return 0;
	case 245:
	case 246:
	case 247:
	case 248:
	case 249: // because we mixed up block shape with block graphics
	case 250:
	case 251:
	case 252:
	case 253:
	case 254:
	case 255: return 255;
	case 242: return 226;
	case 241: return 193;
	case 243: return 227;
	case 240: return 192;
	case 244: return 228;
	case 212: return 196;
	case 196: return 244;
	case 225: return 241;
	case 226: return 210;
	case 224: return 240;
	case 227: return 211;
	case 228: return 212;
	case 210: return 194;
	case 209: return 225;
	case 211: return 195;
	case 208: return 224;
	case 193: return 209;
	case 194: return 242;
	case 192: return 208;
	case 195: return 243;
	default:  return 248;
	}
}
*/




//need function or macro to get a block in the chunk for any i
//handling the out of bounds cases by returning zero

int getBlock(int i, int j) {
	if(i < 0 || i > 255 || j < 0 || j > 255) return 0;
	return chunk.block[i][j];
}


/*
	The API is like int probeDIRECTION(int x, int y, int * normal);  returning distance and surface normal found

	The implementation is based on probing DOWN on a small base set of profiles. Probing other directions is
	done by rotating the profile and unrotating the results.
*/

int profileDepthBase(int profile, int xmod) {
	switch (profile) {
	case   0: return 0; // nothing
	case   1: return 4096; // full block
	case   2: return 2048; // half block
	case   3: return xmod < 2048 ? 0 : 4096; // right half block
	case   4: return xmod / 2; // floor wedge
	case   5: return xmod / 2 + 2048; // wedge raised up half
	case   6: return xmod < 2048 ? 0 : 2 * (xmod - 2048); // right wedge
	case   7: return xmod < 2048 ? 2 * xmod : 4096; // right wedge pushed left half
	default:
		fprintf(stderr, "%s(%d) profileDepthBase bad base profile (%d)\n", __FILE__, __LINE__, profile);
		exit(1);
	}
}

int profileNormalBase(int profile, int xmod) {
	switch (profile) {
	case   0: return 0;   // nothing
	case   1: return 0;   // full block
	case   2: return 0;   // half block
	case   3: return 0;   // right half block
	case   4: return 302; // floor wedge
	case   5: return 302; // wedge raised up half
	case   6: return xmod < 2048 ? 0 : 722; // right wedge
	case   7: return xmod < 2048 ? 722 : 0; // right wedge pushed left half
	default:
		fprintf(stderr, "%s(%d) profileNormalBase bad base profile (%d)\n", __FILE__, __LINE__, profile);
		exit(1);
	}
}

// get the depth at xmod of any profile
int profileDepth(int profile, int xmod) {
	int xmirror = profile < 0 ? 4096 - xmod : xmod;
	int positive = abs(profile);
	int base = positive > 10 ? positive - 10 : positive;
	return profileDepthBase(base, xmirror) - (positive > 10 ? 4096 : 0);
}

// get the normal at xmod of any profile
int profileNormal(int profile, int xmod) {
	int mirror = profile < 0;
	int xmirror = mirror ? 4096 - xmod : xmod;
	int positive = abs(profile);
	int ceiling = positive > 10;
	int base = ceiling ? positive - 10 : positive;
	int normal = profileNormalBase(base, xmirror);
	if(ceiling) normal += 2048;
	if(mirror) normal = -normal;
	return normal;
}

// rotate a profile 90 degrees clockwise
int rotateProfile(int profile) {
	switch (profile) {
	case 0: return 0;
	case 1: return 1;

	// half block
	case  2: return -3;
	case -3: return 12;
	case 12: return  3;
	case  3: return  2;

	// floor wedge
	case   4: return  -6;
	case  -6: return  15;
	case  15: return -17;
	case -17: return   4;

	// wedge raised up half
	case   5: return  -7;
	case  -7: return  14;
	case  14: return -16;
	case -16: return   5;

	// right wedge
	case   6: return  -4;
	case  -4: return  17;
	case  17: return -15;
	case -15: return   6;

	// right wedge pushed left half
	case   7: return  -5;
	case  -5: return  16;
	case  16: return -14;
	case -14: return   7;

	default:
		fprintf(stderr, "rotateProfile bad profile (%d)\n", profile);
		exit(1);
	}
}

int normalizeNormalCode(int n) {
	while(n >  2048) n -= 4096;
	while(n < -2048) n += 4096;
	return n;
}

// profile for block
int blockProfile(int block) {

	if (block == 0)   return 0;
	if (block >= 245) return 1;

	switch(block){
	case 192: return  16;
	case 193: return  17;
	case 194: return -17;
	case 195: return -16;
	case 196: return   2;
	case 208: return -14;
	case 209: return -15;
	case 210: return  15;
	case 211: return  14;
	case 212: return   3;
	case 224: return   7;
	case 225: return   6;
	case 226: return  -6;
	case 227: return  -7;
	case 228: return  12;
	case 240: return  -5;
	case 241: return  -4;
	case 242: return   4;
	case 243: return   5;
	case 244: return  -3;
	default:  return   1;
	}

}

// main probe algorithm, only down
int surfacePingDown(int xmod, int ymod, int profileUp, int profile, int profileDown, int * normal) {

	int extra1 = 4096 - ymod;
	int extra2;

	int surf = profileDepth(profile, xmod);

	if      (surf < 0 && ymod > surf + 4096) goto go_up;     //we're in a ceiling
	else if (surf <= 0)                      goto go_down;   //we're outside and nothing's below
	else if (surf == 4096)                   goto go_up;     //we're in a solid block
	else                                     goto stay_here; //we're above a surface

go_up:
	extra2 = profileDepth(profileUp, xmod);
	if (extra2 <= 0) {
		*normal = 0;
		return -extra1;
	}
	else {
		*normal = profileNormal(profileUp, xmod);
		return -(extra1 + extra2);
	}

go_down:
	extra2 = profileDepth(profileDown, xmod);
	if (extra2 == 4096 || extra2 < 0) {
		*normal = 0;
		return ymod;
	}
	else {
		*normal = profileNormal(profileDown, xmod);
		return ymod + (4096 - extra2);
	}

stay_here:
	*normal = profileNormal(profile, xmod);
	return ymod - surf;

}

// 4 api methods which probe 4 directions
int probeDown(int x, int y, int * normal) {

	if (x < 0 || y < 0 || x >= 4096*256 || y >= 4096*256) return 9999;

	int i    = x / 4096;
	int j    = y / 4096;
	int xmod = x % 4096;
	int ymod = y % 4096;

//printf("x = %d, y = %d, i = %d, j = %d, xmod = %d, ymod = %d\n", x, y, i, j, xmod, ymod);

	if(ymod == 0){
		j--;
		ymod = 4096;
//printf("j--\n");
	}

	if(xmod == 0 || xmod == 2048){
//printf("xmod++\n");
		xmod++;
	}

	int block1 = getBlock(i, j+1);
	int block2 = getBlock(i, j);
	int block3 = getBlock(i, j-1);

//printf("%d %d blocks = %d %d %d\n", i, j, block1, block2, block3);

	int profile1 = blockProfile(block1);
	int profile2 = blockProfile(block2);
	int profile3 = blockProfile(block3);

//printf("profiles = %d %d %d\n", profile1, profile2, profile3);

	return surfacePingDown(xmod, ymod, profile1, profile2, profile3, normal);

}

int probeRight(int x, int y, int * normal) {

	int i    = x / 4096;
	int j    = y / 4096;
	int xmod = x % 4096;
	int ymod = y % 4096;

	int xmod2 =        ymod;
	int ymod2 = 4096 - xmod;

	if(ymod2 == 0){ // impossible
		i++;
		ymod2 = 4096;
	}

	if(xmod2 == 0 || xmod2 == 2048){
		xmod2++;
	}

	int block1 = getBlock(i-1, j);
	int block2 = getBlock(i,   j);
	int block3 = getBlock(i+1, j);

	int profile1 = rotateProfile(blockProfile(block1));
	int profile2 = rotateProfile(blockProfile(block2));
	int profile3 = rotateProfile(blockProfile(block3));

	int n;
	int ping = surfacePingDown(xmod2, ymod2, profile1, profile2, profile3, &n);

	*normal = normalizeNormalCode(n + 1024);
	return ping;

}

int probeUp(int x, int y, int * normal) {

	int i    = x / 4096;
	int j    = y / 4096;
	int xmod = x % 4096;
	int ymod = y % 4096;

	int xmod2 = 4096 - xmod;
	int ymod2 = 4096 - ymod;

	if(ymod2 == 0){ // impossible
		j++;
		ymod2 = 4096;
	}

	if(xmod2 == 0 || xmod2 == 2048){
		xmod2++;
	}

	int block1 = getBlock(i, j-1);
	int block2 = getBlock(i, j);
	int block3 = getBlock(i, j+1);

	int profile1 = rotateProfile(rotateProfile(blockProfile(block1)));
	int profile2 = rotateProfile(rotateProfile(blockProfile(block2)));
	int profile3 = rotateProfile(rotateProfile(blockProfile(block3)));

	int n;
	int ping = surfacePingDown(xmod2, ymod2, profile1, profile2, profile3, &n);

	*normal = normalizeNormalCode(n + 2048);
	return ping;

}

int probeLeft(int x, int y, int * normal) {

	int i    = x / 4096;
	int j    = y / 4096;
	int xmod = x % 4096;
	int ymod = y % 4096;

	int xmod2 = 4096 - ymod;
	int ymod2 =        xmod;

	if(ymod2 == 0){
		i--;
		ymod2 = 4096;
	}

	if(xmod2 == 0 || xmod2 == 2048){
		xmod2++;
	}

	int block1 = getBlock(i+1, j);
	int block2 = getBlock(i,   j);
	int block3 = getBlock(i-1, j);

	int profile1 = rotateProfile(rotateProfile(rotateProfile(blockProfile(block1))));
	int profile2 = rotateProfile(rotateProfile(rotateProfile(blockProfile(block2))));
	int profile3 = rotateProfile(rotateProfile(rotateProfile(blockProfile(block3))));

	int n;
	int ping = surfacePingDown(xmod2, ymod2, profile1, profile2, profile3, &n);

	*normal = normalizeNormalCode(n - 1024);
	return ping;

}
