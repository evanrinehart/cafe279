/*
	renderer

	It uses raylib to do graphics. It knows the structure of models.

	It also sources and dispatches user input for effects on the game.

	It also contains the camera transform and screen dimensions stuff.
*/	

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>

#include <linear.h>
#include <symbols.h>

#include <doodad.h>
#include <megaman.h>
#include <chunk.h>
#include <engine.h>

#include <renderer.h>

#include <bsod.h>

#define CELL 16.0 // the size of a cell in pixels
#define REDUCE(x) (floor(((x) + CELL/2) / CELL)) // cell index for world coordinate
#define EXPAND(i) (CELL*(i)) // world coordinate for center of cell at index

vec2 screenToWorld(double screenX, double screenY);
void getViewBounds(double *l, double *r, double *b, double *t);
void drawVerticalRule(double worldX, Color c);
void drawHorizontalRule(double worldY, Color c);
void drawGridVerticals(double spacing, double offset, Color color);
void drawGridHorizontals(double spacing, double offset, Color color);
void drawGrid(double spacing, double offset, Color color);
void drawSegmentC(vec2 a, vec2 b, Color c);
void drawSegment(vec2 a, vec2 b);
void drawSolidBlock(int i, int j, Color c);
void drawLabel(vec2 p, const char *txt);
void drawBall(vec2 p, double r, Color c);
void drawSprite(Texture tex, vec2 p, int flip);
void drawUISprite(Texture tex, double x, double y, double zoom);

void drawSpriteBase(Texture tex, vec2 p, int flip);

void drawMegaman(vec2 p, int flip);


extern struct Screen {
	int w;
	int h;
	vec2 mouse;
	int UIscale;
} screen;

extern struct World {
	vec2 mouse;
} world;

static Font errorFont;
static Texture errorIcon;

vec2 camera = {0,0};
double zoom = 1;

enum Symbol overlayMode;

int mouse_buttons[10];

struct Screen screen;
struct World world;

struct Megaman megaman;

//static Texture bgtex;
static Texture mmtex;
static Texture statstex;
static Texture tilesetTex;


vec2 updateMouse(){
	Vector2 m = GetMousePosition();
	screen.mouse = vec2(m.x, m.y);
	world.mouse = screenToWorld(m.x, m.y);
	return screen.mouse;
}


int initializeWindow(int w, int h, const char* title){

	screen.w = w;
	screen.h = h;
	screen.UIscale = w / (1920 / 2);

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_VSYNC_HINT);
	//SetTargetFPS(60);

	InitWindow(w, h, title);

	errorFont = LoadFont("assets/fonts/romulus.png");
	errorIcon = LoadTexture("assets/deadgame.png");

	updateMouse();

	return 0;
}

int windowShouldClose(){
	return WindowShouldClose();
}

int loadAssets(){

	mmtex = LoadTexture("assets/megaman.png");
	SetTextureWrap(mmtex, TEXTURE_WRAP_CLAMP);
	megaman.height = mmtex.height;

	statstex = LoadTexture("assets/status-mock.png");
	SetTextureWrap(statstex, TEXTURE_WRAP_CLAMP);

	tilesetTex = LoadTexture("assets/tileset.png");
	SetTextureWrap(tilesetTex, TEXTURE_WRAP_CLAMP);

	return 0;
}


Vector2 worldToScreen(vec2 p){
	double x = floor( zoom * p.x + screen.w/2 - zoom * camera.x);
	double y = floor(-zoom * p.y + screen.h/2 + zoom * camera.y);
	Vector2 v = {x,y};
	return v;
}

vec2 screenToWorld(double screenX, double screenY){
	double x = (screenX + zoom*camera.x - screen.w/2) / zoom;
	double y = (screenY - zoom*camera.y - screen.h/2) / -zoom;
	vec2 v = {x,y};
	return v;
}

void getViewBounds(double *l, double *r, double *b, double *t){
	*l = camera.x - screen.w/2/zoom;
	*r = camera.x + screen.w/2/zoom;
	*b = camera.y - screen.h/2/zoom;
	*t = camera.y + screen.h/2/zoom;
}

void drawVerticalRule(double worldX, Color c){
	vec2 p = {worldX, 0};
	double x = worldToScreen(p).x + 0.5;

	Vector2 a = {x, screen.h};
	Vector2 b = {x, 0};

	DrawLineV(a, b, c);
}

void drawHorizontalRule(double worldY, Color c){
	vec2 p = {0, worldY};
	double y = worldToScreen(p).y + 0.5;

	Vector2 a = {screen.w, y};
	Vector2 b = {0, y};

	DrawLineV(a, b, c);
}

void drawGridVerticals(double spacing, double offset, Color color){
	double xmin, xmax, ymin, ymax;
	getViewBounds(&xmin, &xmax, &ymin, &ymax);

	double W = xmax - xmin;
	int n = floor( W / spacing ) + 1;
	int x0 = floor(xmin / spacing) * spacing;

	for(int i=0; i <= n; i++){
		drawVerticalRule(x0 + i * spacing + offset, color);
	}
}

void drawGridHorizontals(double spacing, double offset, Color color){
	double xmin, xmax, ymin, ymax;
	getViewBounds(&xmin, &xmax, &ymin, &ymax);

	double H = ymax - ymin;
	int n = floor( H / spacing ) + 1;
	int y0 = floor(ymin / spacing) * spacing;

	for(int i=0; i <= n; i++){
		drawHorizontalRule(y0 + i * spacing + offset, color);
	}
}

void drawGrid(double spacing, double offset, Color color){
	drawGridVerticals(spacing, offset, color);
	drawGridHorizontals(spacing, offset, color);
}

void drawSegmentC(vec2 a, vec2 b, Color c){
	Vector2 screen_a = worldToScreen(a);
	Vector2 screen_b = worldToScreen(b);
	screen_a.x += 0.5;
	screen_a.y += 0.5;
	screen_b.x += 0.5;
	screen_b.y += 0.5;
	DrawLineEx(screen_a, screen_b, 1, c);
}

void drawSegment(vec2 a, vec2 b){
	drawSegmentC(a, b, MAGENTA);
}

void drawSolidBlock(int i, int j, Color c){
	vec2 p = {EXPAND(i) - CELL/2, EXPAND(j) + CELL/2};
	Vector2 screen = worldToScreen(p);
	DrawRectangle(screen.x, screen.y, CELL*zoom, CELL*zoom, c);
}

void drawLabel(vec2 p, const char *txt){
	Vector2 screen = worldToScreen(p);
	DrawText(txt, screen.x, screen.y, zoom, BLACK);
}

void drawBall(vec2 p, double r, Color c){
	Vector2 screen = worldToScreen(p);
	DrawCircle(screen.x, screen.y, r*zoom, c);
}

void drawSprite(Texture tex, vec2 p, int flip){
	vec2 wcorner = add((vec2){-tex.width / 2, tex.width / 2}, p);
	Vector2 scorner = worldToScreen(wcorner);
	double sign = flip ? -1 : 1;

	Rectangle src = {0, 0, sign*tex.width, tex.height};
	Rectangle dst = {scorner.x, scorner.y, tex.width*zoom, tex.height*zoom};
	Vector2 zero = {0,0};

	DrawTexturePro(tex, src, dst, zero, 0.0, WHITE);
}

void drawSpriteBase(Texture tex, vec2 p, int flip){
	vec2 wcorner = {p.x - tex.width/2, p.y + tex.height};
	Vector2 scorner = worldToScreen(wcorner);

	Rectangle src = {0, 0, tex.width, tex.height};
	Rectangle dst = {scorner.x, scorner.y, tex.width*zoom, tex.height*zoom};
	Vector2 zero = {0,0};

	DrawTexturePro(tex, src, dst, zero, 0.0, WHITE);
}

void drawUISprite(Texture tex, double x, double y, double zoom){
	Vector2 corner = {x, y};
	DrawTextureEx(tex, corner, 0.0, zoom, WHITE);
}




/* Controller */

void initializeEverything(){
	initializeRooms();
}

// raw action routines... master controller

void mouseMotion(vec2 mouse, vec2 delta){
	// mouse_diff might be large for reasons, don't assume it's small
	int x = mouse.x;
	int y = mouse.y;
	int dx = delta.x;
	int dy = delta.y;
	printf("mouse motion by %d %d to %d %d\n", dx, dy, x, y);




	/* pan service code: */
	if(mouse_buttons[2]){
		camera.x -=  delta.x/zoom;
		camera.y -= -delta.y/zoom;
	}
	/* end of pan code. */
}

void leftClick(vec2 p, int down){
	mouse_buttons[0] = down;

	vec2 q = screenToWorld(p.x, p.y);
	if(down){
		struct Doodad * d = findDoodad(q);
		if(d){ clickDoodad(d); }
	}
	//printf("mouse left click %d at %lf %lf\n", down, p.x, p.y);
}

void rightClick(vec2 p, int down){
	mouse_buttons[1] = down;

	printf("mouse right click %d\n", down);
}

void middleClick(vec2 p, int down){
	mouse_buttons[2] = down;

	printf("mouse middle click %d\n", down);
}

void mouseWheel(vec2 p, double diff){
	if(diff < 0) zoom /= 2;
	if(diff > 0) zoom *= 2;
}

void inputCharacter(int c){
	printf("input character %c\n", c);
}

void pressG(){
	printf("pressed G (%d)\n", KEY_G);
}

void pressKeypad(int n){
	if(n==0) overlayMode = 0;
	if(n==1) overlayMode = ATMOSPHERIC_EDGE_OVERLAY;
	if(n==2) overlayMode = ROOM_BOUNDARY_OVERLAY;
}


// source event from raylib

void dispatchInput(){
	// mouse
	vec2 mouse_prev = screen.mouse;
	vec2 mouse_new = updateMouse();

	if(vec2neq(mouse_new, mouse_prev)){
		vec2 mouse_diff = sub(mouse_new, mouse_prev);
		mouseMotion(mouse_new, mouse_diff);
	}

	vec2 mouse = mouse_new;

	if(IsMouseButtonPressed(0)){ leftClick(mouse, 1); }
	if(IsMouseButtonReleased(0)){ leftClick(mouse, 0); }
	if(IsMouseButtonPressed(1)){ rightClick(mouse, 1); }
	if(IsMouseButtonReleased(1)){ rightClick(mouse, 0); }
	if(IsMouseButtonPressed(2)){ middleClick(mouse, 1); }
	if(IsMouseButtonReleased(2)){ middleClick(mouse, 0); }

	double amount = GetMouseWheelMove();
	if(amount < 0 || 0 < amount){ mouseWheel(mouse, amount); }


	// keyboard
	if(IsKeyPressed(KEY_LEFT_CONTROL)){
		pressG();
	}

	int c = 0;
	while((c = GetCharPressed())){
		inputCharacter(c);
	}

	if(IsKeyPressed(KEY_KP_0)){ pressKeypad(0); }
	if(IsKeyPressed(KEY_KP_1)){ pressKeypad(1); }
	if(IsKeyPressed(KEY_KP_2)){ pressKeypad(2); }
	if(IsKeyPressed(KEY_KP_3)){ pressKeypad(3); }
	if(IsKeyPressed(KEY_KP_4)){ pressKeypad(4); }
	if(IsKeyPressed(KEY_KP_5)){ pressKeypad(5); }
	if(IsKeyPressed(KEY_KP_6)){ pressKeypad(6); }
	if(IsKeyPressed(KEY_KP_7)){ pressKeypad(7); }
	if(IsKeyPressed(KEY_KP_8)){ pressKeypad(8); }
	if(IsKeyPressed(KEY_KP_9)){ pressKeypad(9); }
	if(IsKeyPressed(KEY_BACKSLASH)){ bsod("FATAL EXCEPTION"); }

/*
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

		if(0 && IsMouseButtonPressed(0)){
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

		if(0 && IsMouseButtonPressed(1)){
			struct vec2 xy = screen_to_world(GetMousePosition());
			int i, j;
			pointToTile(xy, &i, &j);

			if(tileAt(i,j)){
				deleteBlock(i+128, j+128);
			}
		}

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

*/


}



/* Renderer */

Rectangle tileIndexToRect(int i){
	int row = i / 16;
	int col = i % 16;
	return (Rectangle){col*16, row*16, 16, 16};
} 

void drawTile(int tile, int i, int j){
	
	vec2 center = {16*i, 16*j};
	double dest_w = zoom*16;
	double dest_h = zoom*16;
	Vector2 corner = worldToScreen(center);
	corner.x -= dest_w / 2;
	corner.y -= dest_h / 2;
	Rectangle sourceRect = tileIndexToRect(tile);
	Rectangle destRect = (Rectangle){corner.x, corner.y, dest_w, dest_h};
	Vector2 vzero = (Vector2){0,0};
	DrawTexturePro(tilesetTex, sourceRect, destRect, vzero, 0.0, WHITE);

}

void renderTiles(){
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

			int tile = chunk.block[i][j];

			if(tile == 0) continue;

			drawTile(tile, i - 128, j -128);
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
//if(west || south) printf("atmospheric edge at %d %d, west=%d, south=%d\n", i, j, west, south);
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
			drawLabel((vec2){x-4,y+4}, TextFormat("%d", dat));
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
			struct Room *ptr = &rooms[r];
			//drawLabel(TextFormat("%d", dat), (vec2){x-4,y+4});
			double pressure = r==1 ? chunk.outsideAirPerCell : (1.0*ptr->air / ptr->volume);

			Color c = {0,0,0,pressure/1000.0 * 255.0};
			if(pressure > 1000.0){ c.a = 255; };

			drawSolidBlock(i-128, j-128, c);
		}
	}
}






void drawDoodad(struct Doodad *d){
	Color c;
	switch(d->symbol){
		case NULL_SYMBOL:              c = BLACK; break;
		case ATMOSPHERIC_EDGE_OVERLAY: c = MAGENTA; break;
		case ROOM_BOUNDARY_OVERLAY:    c = SKYBLUE; break;
		case SYM3:                     c = DARKGREEN; break;
		case SYM4:                     c = GOLD; break;
		case SYM5:                     c = MAROON; break;
		default:                       c = ORANGE;

	}
	drawBall(d->pos, 5, c);

	vec2 offset = {4, -4};
	drawLabel(add(offset, d->pos), d->label);
}

void drawMegaman(vec2 p, int flip){
	drawSpriteBase(mmtex, p, flip);
}

void rerenderEverything(){
	BeginDrawing();

	ClearBackground(WHITE);

	/* * grid * */
	Color c00 = {250,250,250,255};
	Color c0 = {235,235,235,255};
	//Color c1 = {230,230,230,255};
	Color c2 = {200,200,200,255};
	Color c3 = {128,128,128,255};

	if(zoom >= 3) drawGrid(1, 0, c00);       // microgrid
	if(zoom >= 2) drawGrid(16, 8, c0);       // tile grid
	if(zoom > 1.0/16) drawGrid(16*8, 0, c2); // major grid lines i.e. one per cell wall.
	drawVerticalRule(0, c3);
	drawHorizontalRule(0, c3);


	/* * tiles * */ // ???
	renderTiles();
	//for(int i=0; i<db_pieces_ptr; i++){
	//	const struct placed_piece *ptr = &db_pieces[i];
	//	drawTileEx(tilesetTex, ptr->tile_ix, ptr->i, ptr->j);
	//}

	/* * megaman * */ //???
	drawMegaman(vec2(megaman.x, 8), megaman.facing < 0);

	/* * mock status UI * */
	//drawUISprite(statstex, db_config.stats_pos_x, screen_h - db_config.stats_pos_y, 2.0);


	/* doodads */
	for(struct Doodad *d = doodads; d < doodad_ptr; d++){ drawDoodad(d); }

	/* * debug text * */
	DrawFPS(0,0);
	DrawText(TextFormat("Zoom = %lf", zoom), 1, 20, 10, BLACK);


	//renderPressureOverlay();
	renderRoomOverlay();
	if(overlayMode > 0) renderEdgeOverlay(overlayMode);

	EndDrawing();
}


void shutdownEverything(){
	CloseWindow();
}

/* Display BSOD screen until user presses a key to exit */
void bsod(const char* finalMsg){

	fprintf(stderr, "finalMsg: %s\n", finalMsg);

	int scale = screen.UIscale;

	int padding = scale * 100;
	int border = scale * 8;
	int margin = scale * 50;
	//int iconsize = scale * 50;

	int textsize = scale * 24;
	int msgsep = scale * 54;
	int msganchor = scale * 12;

	int rx = padding;
	int ry = padding;
	int rw = screen.w - 2*padding;
	int rh = screen.h - 2*padding;
	//int rxx = rx + rw;
	int ryy = ry + rh;
	int innerX = rx + 2*border;
	//int innerY = ry + 2*border;
	int innerYY = ryy - 2*border;

	int cx = screen.w / 2;
	//int cy = screen.h / 2;

	int iconW = scale * errorIcon.width / 2;
	int iconH = scale * errorIcon.height / 2;

	Rectangle source = {0, 0, errorIcon.width, errorIcon.height};
	Rectangle dest = {cx - iconW/2, ry + rh / 3 - iconH/2, iconW, iconH};
	Vector2 zero = {0,0};

	for(;;){
		BeginDrawing();
			DrawRectangle(rx, ry, rw, rh, BLACK);
			DrawRectangle(rx+border, ry+border, rw-2*border, rh-2*border, RED);
			DrawRectangle(rx+2*border, rx+2*border, rw-4*border, rh-4*border, BLACK);
			DrawTexturePro(errorIcon, source, dest, zero, 0.0, WHITE); // Draw a part of a texture defined by a rectangle with 'pro' parameters
			Vector2 p1 = {innerX + margin, innerYY - margin - msganchor - msgsep};
			Vector2 p2 = {innerX + margin, innerYY - margin - msganchor };
			DrawTextEx(errorFont, finalMsg, p1, textsize, 4, RED);
			DrawTextEx(errorFont, "Press any key to continue", p2, textsize, 4, RED);
		EndDrawing();

/*
		if(IsKeyDown(KEY_SPACE)) exit(1);
		if(IsKeyDown(KEY_UP)){   msgsep++; printf("value = %d\n", msgsep); }
		if(IsKeyDown(KEY_DOWN)){ msgsep--; printf("value = %d\n", msgsep); }
		if(IsKeyDown(KEY_LEFT)){ msganchor++; printf("value = %d\n", msganchor); }
		if(IsKeyDown(KEY_RIGHT)){ msganchor--; printf("value = %d\n", msganchor); }
*/

		if(GetKeyPressed()) exit(1);
	}
}

/* Enter the bsod loop but EndDrawing first. Usable inside renderer */
void bsodED(const char* finalMsg){
	EndDrawing();

	bsod(finalMsg);
}
