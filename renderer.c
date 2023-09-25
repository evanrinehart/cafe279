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
#include <engine.h>
#include <doodad.h>
#include <megaman.h>
#include <items.h>
#include <chunk.h>
#include <renderer.h>
#include <bsod.h>
#include <physics.h> // temporary
#include <sound.h>
#include <brain.h>

#include <misc.h>

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
void drawUISprite(Texture tex, double x, double y, double zoom);

void drawSpriteBase(Texture tex, vec2 p, int flip);

void drawMegaman(vec2 p, int flip);

enum Overlay {
	NO_OVERLAY,
	ATMOSPHERIC_EDGE_OVERLAY,
	ROOM_BOUNDARY_OVERLAY
};

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

enum Overlay overlayMode;

int mouse_buttons[10];

struct Screen screen;
struct World world;

//static Texture bgtex;
static Texture mmtex;
static Texture statstex;
static Texture tilesetTex;

enum Tool {BLOCK_EDIT_TOOL};

enum Tool tool = BLOCK_EDIT_TOOL;

int blockChoice = 250;

vec2 updateMouse(){
	Vector2 m = GetMousePosition();
	screen.mouse = vec2(m.x, m.y);
	world.mouse = screenToWorld(m.x, m.y);
	return screen.mouse;
}

int initializeWindow(int w, int h, const char* title){

	fprintf(stderr, "%s(%d) initializeWindow...\n", __FILE__, __LINE__);

	screen.w = w;
	screen.h = h;
	screen.UIscale = w / (1920 / 2);

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	if (engine.vsync) SetConfigFlags(FLAG_VSYNC_HINT);
	//SetTargetFPS(60);

	InitWindow(w, h, title);

	SetWindowSize(w,h);

	errorFont = LoadFont("assets/fonts/romulus.png");
	errorIcon = LoadTexture("assets/deadgame.png");

	updateMouse();

	InitAudioDevice();

	engine.videoEnabled = true;

	return 0;
}

int windowShouldClose(){
	return WindowShouldClose();
}

static int icon_count = 0;
//static Texture icons[256];
static Font infoFont;
static Font infoFont2;
static Font infoFont3;

//static const char* fontpath = "assets/fonts/RobotoCondensed-Regular.ttf";
static int F = 13;

static Texture circuitTex;
static Texture circuitTexBL;

int loadAssets(){

	fprintf(stderr, "%s(%d) loadAssets...\n", __FILE__, __LINE__);

	FilePathList fpl = LoadDirectoryFiles("assets/icons");
	for (unsigned i = 0; i < fpl.count; i++) {
		//icons[i] = LoadTexture(fpl.paths[i]);
		icon_count++;
	}

	//infoFont  = LoadFontEx(fontpath, F, NULL, 250);
	//infoFont2 = LoadFontEx(fontpath, 2*F, NULL, 250);
	//infoFont3 = LoadFontEx(fontpath, 3*F, NULL, 250);

	mmtex = LoadTexture("assets/megaman.png");
	SetTextureWrap(mmtex, TEXTURE_WRAP_CLAMP);
	megaman.height = mmtex.height;

	// hacked trilinear settings looks good when items are not actively rotating
	circuitTex = LoadTexture("assets/icons/circuitboard.png");
	GenTextureMipmaps(&circuitTex);
	SetTextureFilter(circuitTex, TEXTURE_FILTER_TRILINEAR);

	// a separate bilinear filtered version can be used on rotating loose items
	circuitTexBL = LoadTexture("assets/icons/circuitboard.png");
	GenTextureMipmaps(&circuitTexBL);
	SetTextureFilter(circuitTexBL, TEXTURE_FILTER_BILINEAR);

	statstex = LoadTexture("assets/status-mock.png");
	SetTextureWrap(statstex, TEXTURE_WRAP_CLAMP);

	tilesetTex = LoadTexture("assets/tileset.png");
	SetTextureWrap(tilesetTex, TEXTURE_WRAP_CLAMP);

	loadSounds();

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

void drawSolidBlock(int i, int j, Color c) {
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

void drawSprite(Texture tex, vec2 p, double scale, double rotation){
	double w = scale * tex.width;
	double h = scale * tex.height;

	vec2 offset = {-w / 2, h / 2};
	vec2 wcorner = add(offset, p);

	Vector2 scorner = worldToScreen(wcorner);
	Vector2 scenter = worldToScreen(p);

	Vector2 origin = {scenter.x - scorner.x, scenter.y - scorner.y};

	Rectangle src = {0, 0, tex.width, tex.height};
	Rectangle dst = {scenter.x, scenter.y, w*zoom, h*zoom};

	DrawTexturePro(tex, src, dst, origin, rotation, WHITE);
}

void drawSpriteWidth(Texture tex, vec2 p, double width, double rotation){
	double aspect = tex.height / tex.width;
	double w = width;
	double h = aspect * w;

	vec2 offset = {-w / 2, h / 2};
	vec2 wcorner = add(offset, p);

	Vector2 scorner = worldToScreen(wcorner);
	Vector2 scenter = worldToScreen(p);

	Vector2 origin = {scenter.x - scorner.x, scenter.y - scorner.y};

	Rectangle src = {0, 0, tex.width, tex.height};
	Rectangle dst = {scenter.x, scenter.y, w*zoom, h*zoom};

	DrawTexturePro(tex, src, dst, origin, rotation, WHITE);
}

void drawSpriteH(Texture tex, vec2 p, int flip){
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

void mouseMotion(vec2 mouse, vec2 delta){
	// mouse_diff might be large for reasons, don't assume it's small

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
		if(d){ clickDoodad(d); return; }
	}

	if(down && tool == BLOCK_EDIT_TOOL){
		puts("put block");
		int i = REDUCE(q.x) + 128;
		int j = REDUCE(q.y) + 128;
		putBlock(i, j, blockChoice);
		return;
	}

}

void rightClick(vec2 mouse, int down){
	mouse_buttons[1] = down;

	vec2 p = screenToWorld(mouse.x, mouse.y);

	if(down && tool == BLOCK_EDIT_TOOL){
		puts("erase block");
		int i = REDUCE(p.x) + 128;
		int j = REDUCE(p.y) + 128;
		eraseBlock(i, j);
	}

}

void middleClick(vec2 mouse, int down){
	mouse_buttons[2] = down;
}

int zoomLevel = 0;

void mouseWheel(vec2 mouse, double diff){
	if (diff < 0) zoomLevel--;
	if (diff > 0) zoomLevel++;

	if (zoomLevel > 6) zoomLevel = 6;
	if (zoomLevel < 0) zoomLevel = 0;

	switch (zoomLevel) {
		case -3: zoom = 0.125; break;
		case -2: zoom = 0.25; break;
		case -1: zoom = 0.5; break;
		case 0: zoom = 1.0; break;
		case 1: zoom = 2.0; break;
		case 2: zoom = 4.0; break;
		case 3: zoom = 6.0; break;
		case 4: zoom = 12.0; break;
		case 5: zoom = 24.0; break;
		case 6: zoom = 48.0; break;
		default: zoom = 1.0;
	}
}

void pressPause(){
	engine.paused = !engine.paused;
}

void pressKeypad(int n){
	if(n==0) overlayMode = NO_OVERLAY;
	if(n==1) { printf("atmo edge overlay\n"); overlayMode = ATMOSPHERIC_EDGE_OVERLAY; }
	if(n==2) { printf("room boundary overlay\n"); overlayMode = ROOM_BOUNDARY_OVERLAY; }
}

void holdUpDownArrow(vec2 mouse, int updown){
	vec2 p = screenToWorld(mouse.x, mouse.y);
	int i = REDUCE(p.x);
	int j = REDUCE(p.y);
	int rid = chunk.room[i+128][j+128];
	struct Room *room = roomById(rid); if(room == NULL) return;
	room->air += updown * 50;
	if(room->air < 0) room->air = 0;
	if(updown > 0) printf("room %d pump it up! %d\n", room->id, room->air);
	else           printf("room %d deflating! %d\n", room->id, room->air);
}




void pressV(){
	engine.vsyncHint = !engine.vsync;
}

void manageVSync(){
	if (engine.vsyncHint != engine.vsync) {
		if(engine.vsyncHint){
			SetWindowState(FLAG_VSYNC_HINT);
			engine.vsync = true;
		}
		else{
			ClearWindowState(FLAG_VSYNC_HINT);
			engine.vsync = false;
		}
	}
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
	if(IsKeyPressed(KEY_G)){ pressG(); }
	if(IsKeyPressed(KEY_R)){ pressR(); }
	if(IsKeyPressed(KEY_PAUSE)){ pressPause(); }
	if(IsKeyPressed(KEY_H)){ pressH(); }
	if(IsKeyPressed(KEY_C)){ pressC(); }
	if(IsKeyPressed(KEY_N)){ pressN(); }
	if(IsKeyPressed(KEY_L)){ pressL(); }
	if(IsKeyPressed(KEY_P)){ pressP(); }
	if(IsKeyPressed(KEY_V)){ pressV(); }

	if(IsKeyPressed(KEY_I)){ pressI(); }

	if(IsKeyDown(KEY_UP))  { holdUpDownArrow(mouse,  1); }
	if(IsKeyDown(KEY_DOWN)){ holdUpDownArrow(mouse, -1); }

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
	if(IsKeyPressed(KEY_BACKSLASH)){ bsod("FATAL EXCEPTION (\\)"); }

/*

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


void renderEdgeOverlay(enum Overlay mode){
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
			else if(mode == ROOM_BOUNDARY_OVERLAY){ //ROOM_BOUNDARY_OVERLAY
				west = chunk.room_edges_v[i][j];
				south = chunk.room_edges_h[i][j];
			}
			else{
				return;
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

			int rid = chunk.room[i][j];
			if(rid < 1) continue;
			struct Room *ptr = roomById(rid);
			if(ptr == NULL) continue;
			//drawLabel(TextFormat("%d", dat), (vec2){x-4,y+4});
			double pressure = rid==1 ? chunk.outsideAirPerCell : (1.0*ptr->air / ptr->volume);

			Color c = {0,0,0,pressure/1000.0 * 255.0};
			if(pressure > 1000.0){ c.a = 255; };

			drawSolidBlock(i-128, j-128, c);
		}
	}
}


const char * netstatusToString(int status){
	switch (status) {
	case SERVER: return "SERVER";
	case CLIENT: return "CLIENT";
	case CONNECTING: return "CONNECTING";
	case OFFLINE: return "OFFLINE";
	default: return "badval";
	}
}



void drawObject(struct Object *obj){
	double radius = 4;

	int n = obj - objects;

	double trash;

	double x = modf(n * sqrt(2), &trash);

	Color c = ColorFromHSV(x * 360, 1.0, 1.0);

	//drawSolidBlock(10,10,RED);
	//drawSolidBlock(10,12,RED);
	//drawSolidBlock(10,14,RED);
	//drawSolidBlock(12,10,RED);

	struct CellWindow win = discFootprint(obj->pos, radius);

	for(int i=win.imin; i<=win.imax; i++){
	for(int j=win.jmin; j<=win.jmax; j++){
		//drawSolidBlock(i,j,BLUE);
	}
	}

	drawBall(obj->pos, radius, c);
}


void drawDoodad(struct Doodad *d){
	Color c;
	switch(d->symbol){
		case NULL_SYMBOL:              c = BLACK; break;
		case NEPTUNE_SYMBOL:           c = MAGENTA; break;
		case MOON_SYMBOL:              c = SKYBLUE; break;
		case SUN_SYMBOL:               c = DARKGREEN; break;
		case SATURN_SYMBOL:            c = GOLD; break;
		case GALAXY_SYMBOL:            c = MAROON; break;
		default:                       c = ORANGE;

	}
	drawBall(d->pos, 5, c);

	vec2 offset = {4, -4};
	drawLabel(add(offset, d->pos), d->label);
}

void drawLooseItem(struct LooseItem * item){
	//printf("rendering %p i=%d j=%d\n", (void*)item, item->i, item->j);
	vec2 p = {item->i - 128*16, item->j - 128*16};
	double rot = item->rotation;
	drawSpriteWidth(circuitTexBL, p, 48/12, rot);
}

void drawMegaman(vec2 p, int flip){
	drawSpriteBase(mmtex, p, flip);
}


void renderSwap(){
	SwapScreenBuffer();
}

void renderPollInput(){
	if (engine.inputFresh) return;
	PollInputEvents();
	engine.inputFresh = true;
}

void rerenderEverything(){

	BeginDrawing();

	ClearBackground(WHITE);

	/* * grid * */
	Color c00 = {250,250,250,255};
	Color c0 = {235,235,235,255};
	Color c2 = {200,200,200,255};
	Color c3 = {128,128,128,255};

	if(zoom >= 3) drawGrid(1, 0, c00);       // microgrid
	if(zoom >= 2) drawGrid(16, 8, c0);       // tile grid
	if(zoom > 1.0/16) drawGrid(16*8, 0, c2); // major grid lines i.e. one per cell wall.
	drawVerticalRule(0, c3);
	drawHorizontalRule(0, c3);

	/* * tiles * */
	renderTiles();

	/* * megaman * */
	drawMegaman(vec2(megaman.x, 8), megaman.facing < 0);

	for(struct LooseItem *ptr = looseItems; ptr < looseItems_ptr; ptr++){
		drawLooseItem(ptr);
	}

	/* test circuit board item images */
	drawSpriteWidth(circuitTex, vec2(16 * -5, 16 * 4), 48/3, 0);

	drawSpriteWidth(circuitTex, vec2(16 * -6 - 8 + 4, 16 * 4 - 8 + 4), 48/6, 0);
	drawSpriteWidth(circuitTex, vec2(16 * -6 - 8 + 12, 16 * 4 - 8 + 4), 48/6, 0);
	drawSpriteWidth(circuitTex, vec2(16 * -6 - 8 + 4, 16 * 4 - 8 + 12), 48/6, 0);
	drawSpriteWidth(circuitTex, vec2(16 * -6 - 8 + 12, 16 * 4 - 8 + 12), 48/6, 0);

	for(int j = 1; j <= 4; j ++){
		drawSpriteWidth(circuitTex, vec2(16 * -7 - 8 + 2, 16 * 4 - 8 - 2 + 4*j), 48/12, 0);
		drawSpriteWidth(circuitTex, vec2(16 * -7 - 8 + 6, 16 * 4 - 8 - 2 + 4*j), 48/12, 0);
		drawSpriteWidth(circuitTex, vec2(16 * -7 - 8 + 10, 16 * 4 - 8 - 2 + 4*j), 48/12, 0);
		drawSpriteWidth(circuitTex, vec2(16 * -7 - 8 + 14, 16 * 4 - 8 - 2 + 4*j), 48/12, 0);
	}

	/* * mock status UI * */
	Vector2 stats_pos = {30,110};
	drawUISprite(statstex, stats_pos.x, screen.h - stats_pos.y, 2.0);

	/* doodads */
	//for(struct Doodad *doodad = doodads; doodad < doodads_ptr; doodad++){ drawDoodad(doodad); }
	for(struct Object *obj = objects; obj < objects_ptr; obj++){ drawObject(obj); }

	/* * debug text * */
	DrawFPS(0,0);
	DrawText(TextFormat("Zoom = %lf", zoom), 1, 20, 10, BLACK);
	DrawText(TextFormat("LocalTime = %lf", engine.localTime), 1, 30, 10, BLACK);
	DrawText(TextFormat("ServerTime = %lf", engine.serverTime), 1, 40, 10, BLACK);
	DrawText(TextFormat("TimeOffset = %lf", engine.timeOffset), 1, 50, 10, BLACK);
	DrawText(TextFormat("Paused = %d", engine.paused), 1, 60, 10, BLACK);
	DrawText(TextFormat("FrameNo = %d", engine.frameNumber), 1, 70, 10, BLACK);
	DrawText(TextFormat("NetStatus = %s", netstatusToString(engine.networkStatus)), 1, 80, 10, BLACK);

	//renderPressureOverlay();
	//renderRoomOverlay();
	if(overlayMode > 0) renderEdgeOverlay(overlayMode);

	EndDrawing();

	manageVSync();
}


void bsod(const char * finalMsg) {
	engine.finalMsg = finalMsg;
}

/* Display BSOD screen until user presses a key to exit */
void bsodLoop(const char * finalMsg) {

	fprintf(stderr, "Final message: %s\n", finalMsg);

	if (!engine.videoEnabled) exit(1);

	int scale = screen.UIscale;

	int padding = scale * 100;
	int border = scale * 8;
	int margin = scale * 50;

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
			DrawTexturePro(errorIcon, source, dest, zero, 0.0, WHITE);
			Vector2 p1 = {innerX + margin, innerYY - margin - msganchor - msgsep};
			Vector2 p2 = {innerX + margin, innerYY - margin - msganchor };
			DrawTextEx(errorFont, finalMsg, p1, textsize, 4, RED);
			DrawTextEx(errorFont, "Press any key to continue", p2, textsize, 4, RED);
		EndDrawing();

		SwapScreenBuffer();
		PollInputEvents();

		if(GetKeyPressed()) exit(1);
		if(WindowShouldClose()) exit(1);
	}
}


bool rendererExists() {
	return true;
}

void shutdownRenderer() {
	// TODO unload all the assets here
	CloseWindow();
}
