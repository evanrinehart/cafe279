/* MAIN.C */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#include <raylib.h>
#include <rlgl.h>

#include <sqlite3.h>

#include <linear.h>
#include <floodfill.h>

#include <symbols.h>

#include <grid.h>

#define PI2 6.283185307179586

#define min(X,Y) (X < Y ? X : Y)
#define max(X,Y) (X < Y ? Y : X)

/* TYPES */
struct placed_piece {
	int i;
	int j;
	int layer;
	int tile_ix;
};

struct config {
	double stats_pos_x;
	double stats_pos_y;
};

struct PlayerMove {
	int up;
	int down;
	int right;
	int left;
	int jetpack;
};



/* experimental stuff */
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

struct Chunk chunk;
//int paper[256][256];
//int badoutside[256][256];

struct Bubble {
	int i;
	int j;
	int itimer;
	int jtimer;
	int ispeed;
	int jspeed;
	int amount;
	int substance;
};

struct Room {
	int id;
	int air;
	int volume;
};



#define MAX_ROOMS 1000
struct Room db_rooms[MAX_ROOMS];
int room_counter = 2;



/* ^^^ */





/* GLOBALS */

vec2 camera = {0,0};
int screenW = 1920/2;
int screenH = 1080/2;

double center_x = 0;
double center_y = 0;
double zoom = 1;
int screen_w = 1920/2;
int screen_h = 1080/2;
int frameNumber = 0;

int overlay_mode = 1;

Texture tilesetTex;

struct config db_config;
struct placed_piece db_pieces[1024];
int db_pieces_ptr = 0;

struct PlayerMove playerMove = {0,0,0,0,0};




int pgrid[8][8]; // amount of gas in cell
int dgrid[8][8]; // computed 2nd derivative (divergance)
int xgrid[8][8]; // flow component on cell edge, 0 0 not used
int ygrid[8][8]; // flow component on cell edge
// flow component gives number of units flowing per second.
// in this case, pgrid can never go negative
// sum of pgrid is a constant.


/* pieces DB */

void newPlacedPiece(int i, int j, int layer, int tile_ix){
	struct placed_piece *ptr = &db_pieces[db_pieces_ptr++];
	ptr->i = i;
	ptr->j = j;
	ptr->layer = layer;
	ptr->tile_ix = tile_ix;

}

struct placed_piece * tileAt(int i, int j){
	for(int ix = 0; ix < db_pieces_ptr; ix++){
		struct placed_piece *pp = &db_pieces[ix];
		if(pp->i == i && pp->j == j) return pp;
	}
	return NULL;
}

void deleteTileAt(int i, int j){
	struct placed_piece *pp = db_pieces;
	int c = 0;

	for(c = 0; c < db_pieces_ptr && !(pp->i == i && pp->j == j); c++, pp++);
	for(; c < db_pieces_ptr; db_pieces[c] = db_pieces[c+1], c++);

	db_pieces_ptr--;
}


void verticesOfPlacedPiece(int i, int j, struct vec2 out[4]){
	double cx = i*16;
	double cy = j*16;
	out[0] = (struct vec2){cx + 8, cy + 8};
	out[1] = (struct vec2){cx + 8, cy - 8};
	out[2] = (struct vec2){cx - 8, cy - 8};
	out[3] = (struct vec2){cx - 8, cy + 8};
}

void scribbleBlankConfig(){
	db_config.stats_pos_x = 0;
	db_config.stats_pos_y = 0;
}



// rooms
#define LOOP256(I,J,X) \
	for(int J = 0; J < 256; J++){ \
		for(int I = 0; I < 256; I++){ \
			do{ X; } while(0); \
		} \
	}

void refreshRoomEdges(int i, int j){
	int r = chunk.room[i][j];
	int r0 = chunk.room[i+1][j];
	int r1 = chunk.room[i][j-1];
	int r2 = chunk.room[i-1][j];
	int r3 = chunk.room[i][j+1];

	chunk.room_edges_v[i+1][j] = r != r0;
	chunk.room_edges_h[i][j] =   r != r1;
	chunk.room_edges_v[i][j] =   r != r2;
	chunk.room_edges_h[i][j+1] = r != r3;
}

void refreshAtmoEdge(int i, int j){
	int here = chunk.block[i][j];

	if(i > 0){
		int west = chunk.block[i-1][j];
		chunk.atmo_edges_v[i][j] = here + west;
	}
	
	if(j > 0){
		int south = chunk.block[i][j-1];
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
	if(r > 1) return (double)db_rooms[r].air / (db_rooms[r].volume + volumeAdjust);
	return 0;
}

int roomExists(int r){
	LOOP256(i,j, if(chunk.room[i][j] == r) return 1;)
	return 0;
}

void showRooms(){
	printf("%10s %10s %10s %10s\n", "id", "air", "volume", "pressure");
	for(int i = 1; i < room_counter; i++){
		struct Room * ptr = &db_rooms[i];
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
	if(r > 0) db_rooms[r].volume = measureVolume(i, j);
}

struct Room * createNewRoom(int i, int j, int air, int volume){
	if(room_counter == MAX_ROOMS) return NULL;

printf("DING new room (%d) created at %d %d\n", room_counter, i, j);

	int k = room_counter++;

	struct Room *ptr = &db_rooms[k];
	ptr->id = k;
	ptr->volume = volume;
	ptr->air = air;

	//floodfill(i, j, k, chunk.room);
	chunk.room[i][j] = k;

	return ptr;
}


int roomsCanMerge(int r1, int r2){
	struct Room *ptr1 = &db_rooms[r1];
	struct Room *ptr2 = &db_rooms[r2];

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

	db_rooms[0].id = 0;
	db_rooms[0].air = 0;
	db_rooms[0].volume = 65535;
	db_rooms[1].id = 1;
	db_rooms[1].air = 0;
	db_rooms[1].volume = 65535;


	// legacy load from db
	LOOP256(i, j,
		if(tileAt(i-128, j-128)){
			chunk.block[i][j] = 1;
			chunk.room[i][j] = 0;
		}
		else{
			chunk.block[i][j] = 0;
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


/* COORDINATE TRANSFORMS */

Rectangle tile_index_to_rect(int i){
	int row = i / 16;
	int col = i % 16;
	return (Rectangle){col*16, row*16, 16, 16};
} 

double world_to_screen_x(double x){
	//return floor(zoom*(x - center_x) + screen_w/2) + 0.5;
	return floor(zoom*x + screen_w/2 - zoom*center_x);
	//return x;
}

double world_to_screen_y(double y){
	//return floor(-zoom*(y - center_y) + screen_h/2) + 0.5;
	return floor(-zoom*y + screen_h/2 + zoom*center_y);
	//return y;
}

struct vec2 screen_to_world(Vector2 screen_xy){
	double x = (screen_xy.x + zoom*center_x - screen_w/2) / zoom;
	double y = (screen_xy.y - zoom*center_y - screen_h/2) / -zoom;
	return (struct vec2){x,y};
}

Vector2 world_to_screen(Vector2 world_xy){
	double x = floor(zoom*world_xy.x + screen_w/2 - zoom*center_x);
	double y = floor(-zoom*world_xy.y + screen_h/2 + zoom*center_y);
	return (Vector2){x,y};
}

void pointToTile(struct vec2 p, int *i, int *j){
	*i = floor((p.x + 8.0) / 16.0);
	*j = floor((p.y + 8.0) / 16.0);
}



/* CUSTOM DRAWING */


void drawArrow(struct vec2 center, struct vec2 direction){
	//vec2 a = sub(center, scale(8, direction));
	vec2 a = center;
	vec2 b = add(center, scale(1, direction));
	Vector2 screen_a = world_to_screen((Vector2){a.x, a.y});
	Vector2 screen_b = world_to_screen((Vector2){b.x, b.y});
	screen_a.x += 0.5;
	screen_a.y += 0.5;
	screen_b.x += 0.5;
	screen_b.y += 0.5;
	DrawLineEx(screen_a, screen_b, 1, BLACK);
}

void drawPoint(double x, double y, Color c){
	double screen_x = world_to_screen_x(x);
	double screen_y = world_to_screen_y(y);
	DrawCircle(screen_x, screen_y, 3, c);
}

void draw_vertical_guideline(double x, Color c){
	double screen_x = world_to_screen_x(x) + 0.5;
	DrawLineV((Vector2){screen_x, 0}, (Vector2){screen_x, screen_h}, c);
}

void draw_horizontal_guideline(double y, Color c){
	double screen_y = world_to_screen_y(y) + 0.5;
	DrawLineV((Vector2){0, screen_y}, (Vector2){screen_w, screen_y}, c);
}

void draw_vertical_lines(double spacing, double offset, Color color){
	double view_width = screen_w / zoom;
	double start = center_x - view_width / 2;
	double end = center_x + view_width / 2;
	int n = floor((end - start) / spacing) + 1;
	double x0 = floor(start/spacing)*spacing;
	//printf("start=%lf end=%lf n=%d\n", start, end, n);
	for(int i=0; i <= n; i++){
		draw_vertical_guideline(x0 + i * spacing + offset, color);
	}
}

void draw_horizontal_lines(double spacing, double offset, Color color){
	double view_height = screen_h / zoom;
	double start = center_y - view_height;
	double end = center_y + view_height;
	int n = floor((end - start) / spacing) + 1;
	double y0 = floor(start/spacing)*spacing;
	//printf("start=%lf end=%lf n=%d\n", center_y, start, end, n);
	for(int i=0; i <= n; i++){
		draw_horizontal_guideline(y0 + i * spacing + offset, color);
	}
}

void draw_grid(double space, double offset, Color color){
	draw_vertical_lines(space, offset, color);
	draw_horizontal_lines(space, offset, color);
}

void draw_coords(double x, double y){
	if(zoom < 0.5) return;
	double screen_x = world_to_screen_x(x);
	double screen_y = world_to_screen_y(y);
	int size = zoom;
	int i = floor(x);
	int j = floor(y);
	char buf[16];
	sprintf(buf, "(%d,%d)", i, j);
	DrawText(buf, screen_x, screen_y, size, RED);
}

void draw_test_dot(double x, double y){
	double screen_x = world_to_screen_x(x);
	double screen_y = world_to_screen_y(y);
	double screen_r = 8 * zoom;
	//printf("%lf %lf %lf\n", screen_x, screen_y, screen_r);
	//DrawCircle(screen_x, screen_y, screen_r, BLACK);
	DrawCircleSector((Vector2){screen_x,screen_y}, screen_r, 0, 360, 36, BLACK);
}

void drawBackground(Texture tex){
	double scale = 1.0;
	double screen_x = screen_w/2 - scale*tex.width/2;
	double screen_y = screen_h/2 - scale*tex.height/2;
	//printf("bg coords %lf %lf\n", screen_x, screen_y);
	DrawTextureEx(tex, (Vector2){screen_x,screen_y}, 0.0, scale, WHITE);
}

void drawTile(Texture tex, int i, int j){
	double x = 16 * i;
	double y = 16 * j;
	double screen_x = world_to_screen_x(x) - zoom*tex.width/2;
	double screen_y = world_to_screen_y(y) - zoom*tex.height/2;
	DrawTextureEx(tex, (Vector2){screen_x,screen_y}, 0.0, zoom, WHITE);
}

void drawTileEx(Texture tileset, int ix, int i, int j){
	double x = 16 * i;
	double y = 16 * j;
	double dest_w = zoom*16;
	double dest_h = zoom*16;
	double dest_x = world_to_screen_x(x) - dest_w/2;
	double dest_y = world_to_screen_y(y) - dest_h/2;
	Rectangle source_rect = tile_index_to_rect(ix);
	Rectangle dest_rect = (Rectangle){dest_x, dest_y, dest_w, dest_h};
	//DrawTextureEx(tex, (Vector2){screen_x,screen_y}, 0.0, zoom, WHITE);
	//DrawTextureRec(tileset, r, (Vector2){screen_x,screen_y}, WHITE);
	Vector2 vzero = (Vector2){0,0};
	DrawTexturePro(tileset, source_rect, dest_rect, vzero, 0.0, WHITE);
}




/* LINUX CLOCK */

double getCurrentTime(){
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	double whole = tp.tv_sec;
	double frac = tp.tv_nsec;
	return whole + frac / 1e9;
}

/* RANDOM FLOAT etc */

double randf(){
	int i = rand();
	double x = i;
	return x / INT_MAX;
}

double randi(int a, int b){
	return a + rand() % (b - a + 1);
}



/* DATABASE DUMP AND RESTORE */

int dumpConfigF(sqlite3 *db, sqlite3_stmt *stmt, const char *key, double value){
	/* INSERT INTO config VALUES (?,?); */
	if(sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC)) {
		printf("%s line %d bind_text: %s\n", __FILE__, __LINE__, sqlite3_errmsg(db));
		return 101;
	}
	if(sqlite3_bind_double(stmt, 2, value)) return 102;
	if(sqlite3_step(stmt) != SQLITE_DONE) return 103;
	if(sqlite3_reset(stmt)) return 104;
	return 0;
}

double loadConfigF(sqlite3 *db, sqlite3_stmt *stmt, const char *key){
	/* SELECT * FROM config WHERE key = ?; */
	double out;
	if(sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC)) { printf("%s line %d bind_text\n", __FILE__, __LINE__); exit(1); }
	if(sqlite3_step(stmt) != SQLITE_ROW){ printf("%s line %d step \n", __FILE__, __LINE__); exit(1); }
	out = sqlite3_column_double(stmt,1);
	if(sqlite3_reset(stmt)) { printf("%s line %d reset\n", __FILE__, __LINE__); exit(1); }
	return out;
}

int dumpDatabaseToTemp(const char* tempname){
	int status = 0;
	sqlite3 *db;
	sqlite3_stmt *stmt;

	if(sqlite3_open(tempname, &db)){ status = 1; goto cleanupNothing; }
	if(sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL)){ status = 2; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE placed_pieces(i int, j int, layer int, tile_ex int);", NULL, NULL, NULL)){ status = 3; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE rooms(id int, air int, volume int);", NULL, NULL, NULL)){ status = 33; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE config(key TEXT primary key, value ANY);", NULL, NULL, NULL)){ status = 333; goto cleanupDb; }

	if(sqlite3_prepare(db, "INSERT INTO placed_pieces VALUES (?,?,?,?);", -1, &stmt, NULL)){ status = 4; goto cleanupDb; }

	for(int i = 0; i < db_pieces_ptr; i++){
		struct placed_piece *pp = &db_pieces[i];
		if(sqlite3_reset(stmt)){ status = 5; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 1, pp->i)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 2, pp->j)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 3, pp->layer)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 4, pp->tile_ix)){ status = 6; goto cleanupStmt; }
		if(sqlite3_step(stmt) != SQLITE_DONE){ status = 7; goto cleanupStmt; }
	}

	sqlite3_stmt *stmt2;
	if(sqlite3_prepare(db, "INSERT INTO rooms VALUES (?,?,?);", -1, &stmt2, NULL)){ status = 301; goto cleanupStmt; }

	for(int i = 2; i < room_counter; i++){
		if(!roomExists(i)) continue;
		struct Room *pp = &db_rooms[i];
		if(sqlite3_reset(stmt2)){ status = 305; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 1, pp->id)){ status = 306; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 2, pp->air)){ status = 307; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 3, pp->volume)){ status = 308; goto cleanupStmt2; }
		if(sqlite3_step(stmt2) != SQLITE_DONE){ status = 399; goto cleanupStmt2; }
	}

	sqlite3_stmt *stmt3;
	if(sqlite3_prepare(db, "INSERT INTO config VALUES (?, ?);", -1, &stmt3, NULL)){ status = 202; goto cleanupStmt2; }
	status = dumpConfigF(db, stmt3, "stats_pos_x", db_config.stats_pos_x); if(status){ goto cleanupStmt3; }
	status = dumpConfigF(db, stmt3, "stats_pos_y", db_config.stats_pos_y); if(status){ goto cleanupStmt3; }

	if(sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL)){ status = 8; goto cleanupStmt3; }

	cleanupStmt3:
	sqlite3_finalize(stmt3);
	cleanupStmt2:
	sqlite3_finalize(stmt2);
	cleanupStmt:
	sqlite3_finalize(stmt);
	cleanupDb:
	while(sqlite3_close(db) == SQLITE_BUSY){ printf("too busy to close!\n"); sqlite3_sleep(1000); }
	cleanupNothing:

	return status;
}

int dumpDatabase(const char* outname){
	int status = 0;
	char tempname[64] = "tempsave-XXXXXX";
	int fd;

	fd = mkstemp(tempname);                /* might fail */
	close(fd);                             /* might fail */
	status = dumpDatabaseToTemp(tempname); /* might fail */
	if(status)
		unlink(tempname);                  /* might fail */
	else
		rename(tempname, outname);         /* might fail */

	return status;
}


int loadDatabase(const char* path){
	int status = 0;
	sqlite3 *db;
	sqlite3_stmt *stmt;

	if(sqlite3_open(path, &db)){ status = 1; goto cleanupNothing; }
	if(sqlite3_prepare(db, "SELECT * FROM placed_pieces;", -1, &stmt, NULL)){ status = 2; goto cleanupDb; }

	db_pieces_ptr = 0;
	for(;;){
		int result = sqlite3_step(stmt);
		if(result == SQLITE_ROW){
			newPlacedPiece(
				sqlite3_column_int(stmt,0),
				sqlite3_column_int(stmt,1),
				sqlite3_column_int(stmt,2),
				sqlite3_column_int(stmt,3)
			);
		}
		else if(result == SQLITE_DONE){
			break;
		}
		else{
			status = 3;
			goto cleanupStmt;
		}
	}

	sqlite3_stmt *stmt2;
	if(sqlite3_prepare(db, "SELECT * FROM config WHERE key = ?", -1, &stmt2, NULL)){ status = 201; goto cleanupStmt; }
	db_config.stats_pos_x = loadConfigF(db, stmt2, "stats_pos_x");
	db_config.stats_pos_y = loadConfigF(db, stmt2, "stats_pos_y");
	sqlite3_finalize(stmt2);

	cleanupStmt:
	sqlite3_finalize(stmt);
	cleanupDb:
	while(sqlite3_close(db) == SQLITE_BUSY){ printf("too busy to close!\n"); sqlite3_sleep(1000); }
	cleanupNothing:

	return status;
	
}



/* COLLISION ROUTINE */

struct CollisionResult {
	vec2 point;
	vec2 normal;
	vec2 surface;
};

static struct CollisionResult _result;
static double _D;

void initializeCollisionTest(vec2 a, vec2 b){
	_D = INFINITY;
}

struct CollisionResult * getCollisionResult(){
	return _D < INFINITY ? &_result : NULL;
}

void makeCorners(int i, int j, vec2 out[4]){
	double cx = 16 * i;
	double cy = 16 * j;
	out[0] = (vec2){cx + 8, cy + 8};
	out[1] = (vec2){cx + 8, cy - 8};
	out[2] = (vec2){cx - 8, cy - 8};
	out[3] = (vec2){cx - 8, cy + 8};
}

void collide(vec2 a, vec2 b, int i, int j){
	struct placed_piece *pp;

	if((pp = tileAt(i,j))){
		//printf("yes tile at %d %d\n", i, j);
		vec2 corner[4];
		makeCorners(i,j,corner);

		//printf("surroundings:\n");
		//printf("0 %p:\n", tileAt(i+1, j));
		//printf("1 %p:\n", tileAt(i, j-1));
		//printf("2 %p:\n", tileAt(i-1, j));
		//printf("3 %p:\n", tileAt(i, j+1));

		for(int k = 0; k < 4; k++){
			//printf("  trying k = %d\n", k);
			switch(k){
				case 0: if(tileAt(i+1,j)) continue; break;
				case 1: if(tileAt(i,j-1)) continue; break;
				case 2: if(tileAt(i-1,j)) continue; break;
				case 3: if(tileAt(i,j+1)) continue; break;
			}
			//printf("    edge detected\n");
			vec2 c = corner[k];
			vec2 d = corner[(k + 1) % 4];
			vec2 crossPoint;
			//printf("a = %lf %lf\n", a.x, a.y);
			//printf("b = %lf %lf\n", b.x, b.y);
			//printf("c = %lf %lf\n", c.x, c.y);
			//printf("d = %lf %lf\n", d.x, d.y);
			if(segmentsIntersection(a, b, c, d, &crossPoint)){
				double D = distanceSquaredTo(a, crossPoint);
				//printf("D = sqrt(%lf) = %lf\n", D, sqrt(D));
				//printf("cross = %lf %lf\n", crossPoint.x, crossPoint.y);
				if(D < _D){
					_D = D;
					_result.point = crossPoint;
					_result.normal = normal(d, c);
					_result.surface = sub(d, c);
				}
			}
			else{ 
			//\	printf("no intersection\n");
			}
		}
	}
	
}

vec2 forcedir(int me, int i, int j);

/* look at the path from point(now) to point(now + dt) for any collisions */
/* return collision results or NULL */
void projectile(vec2 *pos, vec2 *vel){
	struct vec2 a = *pos;
	struct vec2 b = add(a, scale(1.0/60, *vel));

	int i0 = REDUCE(a.x);
	int j0 = REDUCE(a.y);
	int i1 = REDUCE(b.x);
	int j1 = REDUCE(b.y);

	initializeCollisionTest(a,b);

	if (i0 == i1 && j0 == j1) {
		//printf("single %d %d \n", i0, j0);
		collide(a, b, i0, j0);
	}
	else if (abs(i0 - i1) + abs(j0 - j1) == 1) {
		//printf("double %d %d %d %d\n", i0, j0, i1, j1);
		collide(a, b, i0, j0);
		collide(a, b, i1, j1);
	}
	else {
		//printf("multi %d %d %d %d\n", i0, j0, i1, j1);
		int imin = min(i0, i1);
		int imax = max(i0, i1);
		int jmin = min(j0, j1);
		int jmax = max(j0, j1);

		printf("pos=%lf %lf vel=%lf %lf\n", pos->x, pos->y, vel->x, vel->y);
		printf("a=%lf %lf b=%lf %lf\n", a.x, a.y, b.x, b.y);
		printf("entering proj loop %d %d %d %d\n", imin, imax, jmin, jmax);
		for(int j = jmin; j <= jmax; j++){
			printf("j = %d\n", j);
			for(int i = imin; i <= imax; i++){
				printf("i = %d\n", i);
				if(i < -1000000) exit(1);
				collide(a, b, i, j);
			}
		}
	}

	//printf("\n");

	struct CollisionResult *col = getCollisionResult();
	if(col){
		//printf("a = %lf %lf \n", a.x, a.y);
		//printf("b = %lf %lf \n", b.x, b.y);
		//printf("v = %lf %lf \n", vel->x, vel->y);
		//printf("point = %lf %lf \n", col->point.x, col->point.y);
		//exit(1);
		*pos = lerp(a, col->point, 0.5);
		//*vel = rotate(reflect(*vel, col->surface), 0.1*(randf()/1.0 - 0.5));
		//*vel = rotate(reflect(*vel, col->surface), 0.5*(randf()/1.0 - 0.5));

		*vel = rotate(reflect(*vel, col->surface), 0.1*randf()); // good for random path finding
	}
	else{
		*pos = b;
	}

}






/* EXPERIMENTAL */

void renderRoomOverlay(){
	double xmin, xmax, ymin, ymax;

	getViewBounds(&xmin, &xmax, &ymin, &ymax);
	
	for(int i = 0; i < 256; i++){
		for(int j = 0; j < 256; j++){
			double x = EXPAND(i - 128);
			double y = EXPAND(j - 128);

			if(x < xmin - 16) continue;
			if(x > xmax + 16) continue;
			if(y < ymin - 16) continue;
			if(y > ymax + 16) continue;

			int dat = chunk.room[i][j];
			//int p = paper[i][j];
			drawLabel(TextFormat("%d", dat), (vec2){x-4,y+4});
			//drawLabel(TextFormat("%d", p), (vec2){x+2,y-2});
		}
	}
}

void renderPressureOverlay(){
	double xmin, xmax, ymin, ymax;

	getViewBounds(&xmin, &xmax, &ymin, &ymax);
	
	for(int i = 0; i < 256; i++){
		for(int j = 0; j < 256; j++){
			double x = EXPAND(i - 128);
			double y = EXPAND(j - 128);

			if(x < xmin - 16) continue;
			if(x > xmax + 16) continue;
			if(y < ymin - 16) continue;
			if(y > ymax + 16) continue;

			int r = chunk.room[i][j];
			if(r < 1) continue;
			struct Room *ptr = &db_rooms[r];
			//drawLabel(TextFormat("%d", dat), (vec2){x-4,y+4});
			double pressure = r==1 ? chunk.outsideAirPerCell : (1.0*ptr->air / ptr->volume);

			Color c = {0,0,0,pressure/1000.0 * 255.0};
			if(pressure > 1000.0){ c.a = 255; };

			drawSolidBlock(i-128, j-128, c);
		}
	}
}

void renderEdgeOverlay(enum Symbol mode){
	double xmin, xmax, ymin, ymax;

	getViewBounds(&xmin, &xmax, &ymin, &ymax);
	
	for(int i = 0; i < 256; i++){
		for(int j = 0; j < 256; j++){
			double x = EXPAND(i - 128);
			double y = EXPAND(j - 128);

			if(x < xmin - 16) continue;
			if(x > xmax + 16) continue;
			if(y < ymin - 16) continue;
			if(y > ymax + 16) continue;

			int west, south;
			if(mode == ATMOSPHERIC_EDGE_OVERLAY){
				west = chunk.atmo_edges_v[i][j];
				south = chunk.atmo_edges_h[i][j];
			}
			else{ //ROOM_BOUNDARY_OVERLAY
				west = chunk.room_edges_v[i][j];
				south = chunk.room_edges_h[i][j];
			}

			vec2 base = {x-8,y-8};

			Color c0 = {0,0,0,255};
			Color c1 = {200,0,0,255};
			Color c2 = {0,200,0,255};
			Color c3 = {0,100,200,255};
			Color c[4] = {c0,c1,c2,c3};
			if(west > 0) drawSegmentC(base, add(base, (vec2){0,16}), c[west]);
			if(south > 0) drawSegmentC(base, add(base, (vec2){16,0}), c[south]);
		}
	}
}




void offsets(int hand, int *di, int *dj){
	*di = hand == 0 ? 1 : (hand == 2 ? -1 : 0);
	*dj = hand == 3 ? 1 : (hand == 1 ? -1 : 0);
}

/*
int computeStart(int i, int j, unsigned mask){
	int di, dj;
	int fallback = -1; // if surrounded by walls, we should not be here

	for(int hand = 0; hand < 4; hand++){
		offsets(hand, &di, &dj);
		if(paper[i+di][j+dj]) continue;
		fallback = fallback==-1 ? hand : fallback;
		if(isOutside(i+di, j+dj)) return hand;
	}

	if(fallback == -1){ printf("computeStart surrounded by walls\n"); exit(1); }
	else return fallback;
}
*/

/*
unsigned computeMask(int i, int j){
	unsigned mask = 0;
	mask |= !!paper[i][j+1]; mask <<= 1;
	mask |= !!paper[i-1][j]; mask <<= 1;
	mask |= !!paper[i][j-1]; mask <<= 1;
	mask |= !!paper[i+1][j];
	return mask ^ 15;
}
*/

int neighboringRoom(int i, int j){
	// assume at least 1 neighboring room, choose any
	int r;
	if((r = chunk.room[i+1][j]) && r != -7) return r;
	if((r = chunk.room[i][j-1]) && r != -7) return r;
	if((r = chunk.room[i-1][j]) && r != -7) return r;
	if((r = chunk.room[i][j+1]) && r != -7) return r;
	printf("neighboringRoom found no neighboring room\n");
	exit(1);
}

/*
int countNeighboringSpaceP(int i, int j){
	int c = 0;
	if(paper[i+1][j] == 0) c++;
	if(paper[i][j-1] == 0) c++;
	if(paper[i-1][j] == 0) c++;
	if(paper[i][j+1] == 0) c++;
	return c;
}
*/

int countNeighboringSpace(int i, int j){
	int c = 0;
	if(chunk.block[i+1][j] == 0) c++;
	if(chunk.block[i][j-1] == 0) c++;
	if(chunk.block[i-1][j] == 0) c++;
	if(chunk.block[i][j+1] == 0) c++;
	return c;
}

int checkContactingRooms(int i, int j){
	int stack[4];
	int ptr = 0;
	int r;

	if((r = chunk.room[i+1][j]) && r != -7) stack[ptr++] = r;
	if((r = chunk.room[i][j-1]) && r != -7) stack[ptr++] = r;
	if((r = chunk.room[i-1][j]) && r != -7) stack[ptr++] = r;
	if((r = chunk.room[i][j+1]) && r != -7) stack[ptr++] = r;

	if(ptr == 0){
		//printf("no rooms at all\n");
		return 0;
	}

	r = stack[0];

//for(int k = 0; k < ptr; k++){
//	printf("  %d\n", stack[k]);
//}
	for(int i = 1; i < ptr; i++){
		if(r != stack[i]){
			//printf("different rooms detected.\n");
			return 1;
		}
	}

	//printf("all rooms the same\n");
	return 0;
}




/*
void healInterfaceBlock(int i, int j){

	if(chunk.room[i][j] != -7) return;

	int neighbors = countNeighboringSpace(i,j);

	if(neighbors > 1 && !checkContactingRooms(i,j)){
		chunk.room[i][j] = neighboringRoom(i,j);
		paper[i][j] = 0;
		return;
	}

	if(neighbors == 1){
		chunk.room[i][j] = neighboringRoom(i,j);
		paper[i][j] = 0;
		return;
	}

	if(neighbors == 0){ // this might be impossible
		paper[i][j] = 0;

		printf("\aWHOA odd room creation\n");

		createNewRoom(i,j,0,1);
		return;
	}

}
*/


/*
void split(int i, int j, unsigned mask, int parentId) {

	const int white = 0;
	const int solid = -127;
	const int color = -20;
	const int iface = -7;

//printf("paper = %d\n", paper[i][j]);

	// assume paper starts with -127 for walls, -7 for interface, 0 for nothing

	if(paper[i][j] == solid) return; // can't split if already solid
	if(paper[i][j] == iface) return; // iface block already separates rooms by definition

	// final pressure is the original rooms pressure if it had one less volume (new wall)
	double pressure = computeRoomPressure(parentId, -1);
	int airStolen = 0;
    
    paper[i][j] = solid; // new wall
	
    int hand = computeStart(i,j,mask);
//printf("5 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
	mask = computeMask(i, j);
//	char buffer[16] = "Hello world";
//	buffer[4] = 0;
//	buffer[3] = '0' + (mask >> 0 & 1);
//	buffer[2] = '0' + (mask >> 1 & 1);
//	buffer[1] = '0' + (mask >> 2 & 1);
//	buffer[0] = '0' + (mask >> 3 & 1);
//printf("split %d %d mask=%s (%d)\n", i, j, buffer, mask);

    int di, dj;

//printf("hand = %d\n", hand);
    offsets(hand, &di, &dj);
    
//printf("di = %d, dj = %d\n", di, dj);
//printf("paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    floodfill(i+di, j+dj, color, paper);
//printf("paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    
    for (int k = 0; k < 3; k++) {

        hand = (hand + 1) % 4;
        offsets(hand, &di, &dj);
//printf("hand = %d\n", hand);
//printf("di = %d, dj = %d, bit=%d\n", di, dj, mask >> hand & 1);
//printf("paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    
        if ((mask >> hand & 1) && paper[i+di][j+dj] == white) {
			floodfill(i+di, j+dj, color, paper);
			int volume = floodvolume(i+di, j+dj, paper);
			int air = pressure * volume;
			airStolen += air;
			createNewRoom(i+di, j+dj, air, volume);
        }
    }

	if(airStolen > 0 && parentId > 1){
		db_rooms[parentId].air -= airStolen;
	}

//printf("0 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    if(mask >> 0 & 1) floodfill(i+1, j, white, paper);
//printf("1 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    if(mask >> 1 & 1) floodfill(i, j-1, white, paper);
//printf("2 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    if(mask >> 2 & 1) floodfill(i-1, j, white, paper);
//printf("3 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);
    if(mask >> 3 & 1) floodfill(i, j+1, white, paper);
//printf("4 paper = %d %d %d %d\n", paper[i+1][j], paper[i][j-1], paper[i-1][j], paper[i][j+1]);


}
*/

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
	newPlacedPiece(i-128, j-128, 0, 250);
	refreshRoomEdges(i, j);
}

void deleteBlock(int i, int j){
	chunk.block[i][j] = 0;
	subAtmoBlockEdges(i, j);
	deleteTileAt(i-128,j-128); // legacy

	int r = accessibleAdjacentRoom(i,j);

	if(r){
		chunk.room[i][j] = r;
	}
	else{
		createNewRoom(i,j,0,1);
	}
		
	refreshRoomEdges(i, j);
}

/*
void newBlock(int i, int j, int flavor){
	printf("\nnewBlock\n");

	int parentId = chunk.room[i][j];

	if(i < 0 || i > 255 || j < 0 || j > 255) return;

	newPlacedPiece(i-128, j-128, 0, flavor);

	chunk.block[i][j] = 255;
	chunk.room[i][j] = 0;
	badoutside[i][j] = -127; // !

	const int unusedFixme = 0;

	if(countNeighboringSpaceP(i,j) > 1){
		printf("hoho\n");
		split(i, j, unusedFixme, parentId);
	}
	else{
		printf("...\n");
		paper[i][j] = -127; // carefully not marking paper if we would split
	}

	healInterfaceBlock(i+1,j);
	healInterfaceBlock(i,j-1);
	healInterfaceBlock(i-1,j);
	healInterfaceBlock(i,j+1);
    
	updateRoomVolume(i+1,j);
	updateRoomVolume(i,j-1);
	updateRoomVolume(i-1,j);
	updateRoomVolume(i,j+1);
}

void deleteBlock(int i, int j){
	printf("\ndeleteBlock");
	printf("chunk.block = %d\n", chunk.block[i][j]);
	if(!chunk.block[i][j]) return;

	int neighboringSpace = countNeighboringSpaceP(i,j);
	printf("neighboring space = %d\n", neighboringSpace);

	deleteTileAt(i-128,j-128); // legacy

	chunk.block[i][j] = 0;

	if(neighboringSpace == 0){
printf("0... neighboringSpace is zero, create a hole\n");
		paper[i][j] = 0;
		badoutside[i][j] = 0;
		chunk.room[i][j] = -1;
		createNewRoom(i, j, 0, 1);
	}
	else if(neighboringSpace == 1){
		chunk.room[i][j] = neighboringRoom(i,j);
printf("single neighboring room = %d\n", chunk.room[i][j]);
		paper[i][j] = 0;
		badoutside[i][j] = 0;
	}
	else if(checkContactingRooms(i,j)){
printf("two+ different rooms meet\n");
		chunk.room[i][j] = -7;
		//paper[i][j] = -127;
		badoutside[i][j] = -7;
	}
	else{
printf("expanding 1 room from multiple directions\n");
		chunk.room[i][j] = neighboringRoom(i,j);
		paper[i][j] = 0;
		badoutside[i][j] = 0;
	}

	updateRoomVolume(i,j);

}
*/







int main(int argc, char* argv[]){

/* LOAD WORKSPACE */

	int loadStatus;
	if((loadStatus = loadDatabase("workspace.save"))){
		printf("load of save failed: %d\n", loadStatus);
		exit(1);
	}



/* SETUP RAYLIB */

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_VSYNC_HINT);
	//SetTargetFPS(60);
	InitWindow(screen_w, screen_h, "TITLE");

	Texture bgtex = LoadTexture("assets/alien-backdrop.png");
	SetTextureFilter(bgtex, TEXTURE_FILTER_BILINEAR);

	//Texture tiletex = LoadTexture("blank-tile.png");
	//SetTextureFilter(tiletex, TEXTURE_FILTER_BILINEAR);

	Texture mmtex = LoadTexture("assets/megaman.png");
	//SetTextureFilter(mmtex, TEXTURE_FILTER_TRILINEAR);
	SetTextureWrap(mmtex, TEXTURE_WRAP_CLAMP);
	//GenTextureMipmaps(&mmtex);

	Texture statstex = LoadTexture("assets/status-mock.png");
	SetTextureWrap(statstex, TEXTURE_WRAP_CLAMP);

	tilesetTex = LoadTexture("assets/tileset.png");
	SetTextureWrap(tilesetTex, TEXTURE_WRAP_CLAMP);



/* SETUP VARIABLES */

	double mmx = 2*16; // dummy megaman state
	double mmv = 16*2;
	int mmfacing = 1;

	/* tile placement tool state var */
	//int newTileIx = 255;

	/* particles state */
	int foo = 0; // pause and unpause physics kludge



	/* ... */

	//fillOutRooms();
	initializeRooms();



	for(;;){ // ... main loop

/* DRAWING */

		BeginDrawing();
		ClearBackground(WHITE);

		/* * grid * */
		Color c00 = {250,250,250,255};
		Color c0 = {235,235,235,255};
		//Color c1 = {230,230,230,255};
		Color c2 = {200,200,200,255};
		Color c3 = {128,128,128,255};

		if(zoom >= 3) draw_grid(1, 0, c00);       // microgrid
		if(zoom >= 2) draw_grid(16, 8, c0);       // tile grid
		if(zoom > 1.0/16) draw_grid(16*8, 0, c2); // major grid lines i.e. one per cell wall.
		draw_vertical_guideline(0, c3);
		draw_horizontal_guideline(0, c3);

		/* * tiles * */
		for(int i=0; i<db_pieces_ptr; i++){
			const struct placed_piece *ptr = &db_pieces[i];
			drawTileEx(tilesetTex, ptr->tile_ix, ptr->i, ptr->j);
		}

		/* * megaman * */
		vec2 mm = {mmx, 16*0.5 + mmtex.height/2.0};
		drawSprite(mmtex, mm, mmfacing < 0);


		/* * mock status UI * */
		drawUISprite(statstex, db_config.stats_pos_x, screen_h - db_config.stats_pos_y, 2.0);

		/* * debug text * */
		DrawFPS(0,0);
		DrawText(TextFormat("Zoom = %lf", zoom), 1, 20, 10, BLACK);


		//renderPressureOverlay();
		renderRoomOverlay();
		if(overlay_mode == 1) renderEdgeOverlay(ATMOSPHERIC_EDGE_OVERLAY);
		if(overlay_mode == 2) renderEdgeOverlay(ROOM_BOUNDARY_OVERLAY);


		EndDrawing();



/* PHYSICS */


		


		//megaman
		if(playerMove.right - playerMove.left < 0) mmfacing = -1;
		if(playerMove.right - playerMove.left > 0) mmfacing = 1;
		mmv = (playerMove.right - playerMove.left) * 100;
		mmx += mmv / 60.0;
		//if(mmx > 16*8){ mmx = 16*8; mmv *= -1; }
		//if(mmx < 16*0.5){ mmx = 16*0.5; mmv *= -1; }

		//pause unpause
		if(IsKeyPressed(KEY_G)){
			foo = !foo;
		}



/* INPUT */

		if(IsKeyPressed(KEY_SPACE)){
			//fillOutRooms();
			initializeRooms();
		}

		if(IsKeyPressed(KEY_G)){
		}


		/* work on player movement control */
		playerMove.up    = IsKeyDown(KEY_W) ? 1 : 0;
		playerMove.down  = IsKeyDown(KEY_S) ? 1 : 0;
		playerMove.right = IsKeyDown(KEY_D) ? 1 : 0;
		playerMove.left  = IsKeyDown(KEY_A) ? 1 : 0;
		playerMove.jetpack = IsKeyDown(KEY_J) ? 1 : 0;

		/* tile paint select: */
		//if(IsKeyDown(KEY_ONE))   newTileIx = 255 - 3;
		//if(IsKeyDown(KEY_TWO))   newTileIx = 255 - 2;
		//if(IsKeyDown(KEY_THREE)) newTileIx = 255 - 1;
		//if(IsKeyDown(KEY_FOUR))  newTileIx = 255 - 0;
		/* end tile paint */

		if(IsKeyDown(KEY_UP)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);
			int r = chunk.room[i+128][j+128];
			db_rooms[r].air += 50;
			printf("pump it up! %d\n", db_rooms[r].air);
		}

		if(IsKeyDown(KEY_DOWN)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);
			int r = chunk.room[i+128][j+128];
			for(int c = 0; c < 50; c++){
				if(db_rooms[r].air > 0){
					db_rooms[r].air -= 1;
					printf("deflating! %d\n", db_rooms[r].air);
				}
			}
		}

		/* pan service code: */
		if(IsMouseButtonDown(2)){
			Vector2 mouse_delta = GetMouseDelta();
			center_x -= mouse_delta.x/zoom;
			center_y -= -mouse_delta.y/zoom;
			camera.x -= mouse_delta.x/zoom;
			camera.y -= -mouse_delta.y/zoom;
		}
		/* end of pan code. */

		/* tile placement and deletion service code */

		if(IsMouseButtonPressed(0)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);
			printf("%d %d\n", i + 128, j + 128);
			//if(tileAt(i,j)) deleteTileAt(i, j);
			if(!tileAt(i,j)){
				//newBlock(i+128, j+128, newTileIx);
				addBlock(i+128, j+128);
			}
			
		}

/*
		if(IsMouseButtonPressed(0)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);
			printf("isOutside(%d,%d) = %d\n", i, j, isOutside(i+128, j+128));
		}
*/
		

		if(IsMouseButtonPressed(1)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);

			if(tileAt(i,j)){
				deleteBlock(i+128, j+128);
			}
		}
		/* end tile placemend service code */

		
		if(IsKeyPressed(KEY_V)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);

			int v = measureVolume(i+128, j+128);
			printf("volume = %d\n", v);
		}

		if(IsKeyPressed(KEY_O)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);

			int o = isOutside(i+128, j+128);
			printf("is outside: %d\n", o);
		}

		if(IsKeyPressed(KEY_R)){
			showRooms();
		}

/*
		static int diff_r1 = -1;
		static int diff_r2 = -1;
		if(IsKeyPressed(KEY_Y)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);

			i += 128;
			j += 128;

			int r = chunk.room[i][j];

			if(r > 0){
				if(diff_r1 < 0){
					diff_r1 = r;
				}
				else{
					diff_r2 = r;
					//mergeTwoRooms(diff_r1, diff_r2);
					//printf("can merge? %d\n", roomsCanMerge(r, diff_r1));
					diff_r1 = -1;
				}
			}
		}
*/

		if(IsKeyDown(KEY_U)){
			//db_rooms[diff_r1].air += 1;
			//db_rooms[diff_r2].air -= 1;
			//printf("can merge? %d\n", roomsCanMerge(diff_r1, diff_r2));
		}

		if(IsKeyPressed(KEY_KP_0)){
			overlay_mode = 0;
		}

		if(IsKeyPressed(KEY_KP_1)){
			overlay_mode = 1;
		}
		
		if(IsKeyPressed(KEY_KP_2)){
			overlay_mode = 2;
		}
		
		/* zoom control */
		double old_zoom = zoom;
		double wheel = GetMouseWheelMove();
		if(wheel < 0) zoom /= 2;
		if(wheel > 0) zoom *= 2;
		if(zoom != old_zoom){
			printf("zoom = %lf\n", zoom);
		}
		/* end zoom control */


/* EPILOGUE */

		if(WindowShouldClose()) break;

		frameNumber++;
	}

	//int dumpStatus = dumpDatabase("workspace.save");
	//printf("dumpDatabase returned %d\n", dumpStatus);

	CloseWindow();

	return 0;

}
