/*
	renderer

	it uses raylib to do graphics. it knows the structure of models.
	other components can call to the renderer for reasons.

	it also contains the camera transform and screen dimensions stuff.
*/	

#include <stdio.h>
#include <math.h>
#include <raylib.h>
#include <linear.h>
#include <symbols.h>

#include <doodad.h>

#include <renderer.h>

vec2 camera = {0,0};
double zoom = 1;
int screenW = 1920/2;
int screenH = 1080/2;

struct Screen screen;
struct World world;


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


void initializeWindow(int w, int h, const char* title){

	screen.w = w;
	screen.h = h;

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_VSYNC_HINT);
	//SetTargetFPS(60);

	InitWindow(w, h, title);

	updateMouse();

}

void loadAssets(){
	//bgtex = LoadTexture("assets/alien-backdrop.png");
	//SetTextureFilter(bgtex, TEXTURE_FILTER_BILINEAR);

	//Texture tiletex = LoadTexture("blank-tile.png");
	//SetTextureFilter(tiletex, TEXTURE_FILTER_BILINEAR);

	mmtex = LoadTexture("assets/megaman.png");
	//SetTextureFilter(mmtex, TEXTURE_FILTER_TRILINEAR);
	SetTextureWrap(mmtex, TEXTURE_WRAP_CLAMP);
	//GenTextureMipmaps(&mmtex);

	statstex = LoadTexture("assets/status-mock.png");
	SetTextureWrap(statstex, TEXTURE_WRAP_CLAMP);

	tilesetTex = LoadTexture("assets/tileset.png");
	SetTextureWrap(tilesetTex, TEXTURE_WRAP_CLAMP);
}

Vector2 worldToScreen(vec2 p){
	double x = floor( zoom * p.x + screenW/2 - zoom * camera.x);
	double y = floor(-zoom * p.y + screenH/2 + zoom * camera.y);
	Vector2 v = {x,y};
	return v;
}

vec2 screenToWorld(double screenX, double screenY){
	double x = (screenX + zoom*camera.x - screenW/2) / zoom;
	double y = (screenY - zoom*camera.y - screenH/2) / -zoom;
	vec2 v = {x,y};
	return v;
}

void getViewBounds(double *l, double *r, double *b, double *t){
	*l = camera.x - screenW/2/zoom;
	*r = camera.x + screenW/2/zoom;
	*b = camera.y - screenH/2/zoom;
	*t = camera.y + screenH/2/zoom;
}

void drawVerticalRule(double worldX, Color c){
	vec2 p = {worldX, 0};
	double x = worldToScreen(p).x + 0.5;

	Vector2 a = {x, screenH};
	Vector2 b = {x, 0};

	DrawLineV(a, b, c);
}

void drawHorizontalRule(double worldY, Color c){
	vec2 p = {0, worldY};
	double y = worldToScreen(p).y + 0.5;

	Vector2 a = {screenW, y};
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

void drawUISprite(Texture tex, double x, double y, double zoom){
	Vector2 corner = {x, y};
	DrawTextureEx(tex, corner, 0.0, zoom, WHITE);
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
	drawBall(d->pos, 5*zoom, c);

	vec2 offset = {4, -4};
	drawLabel(add(offset, d->pos), d->label);
}

void drawMegaman(vec2 p, int flip){
	drawSprite(mmtex, p, flip);
}


// raw action routines

void mouseMotion(vec2 mouse, vec2 mouse_diff){
	// mouse_diff might be large for reasons, don't assume it's small
	int x = mouse.x;
	int y = mouse.y;
	int dx = mouse_diff.x;
	int dy = mouse_diff.y;
	printf("mouse motion by %d %d to %d %d\n", dx, dy, x, y);
}

void leftClick(int down){
	printf("mouse left click %d\n", down);
}

void rightClick(int down){
	printf("mouse right click %d\n", down);
}

void mouseWheel(double w){
	printf("mouse wheel %lf\n", w);
}

void inputCharacter(int c){
	printf("input character %c\n", c);
}

void pressG(){
	printf("pressed G (%d)\n", KEY_G);
}

void dispatchInput(){
	// mouse
	vec2 mouse_prev = screen.mouse;
	vec2 mouse_new = updateMouse();

	if(vec2neq(mouse_new, mouse_prev)){
		vec2 mouse_diff = sub(mouse_new, mouse_prev);
		mouseMotion(mouse_new, mouse_diff);
	}

	if(IsMouseButtonPressed(0)){ leftClick(1); }
	if(IsMouseButtonReleased(0)){ leftClick(0); }
	if(IsMouseButtonPressed(1)){ rightClick(1); }
	if(IsMouseButtonReleased(1)){ rightClick(0); }

	double amount = GetMouseWheelMove();
	if(amount < 0 || 0 < amount){ mouseWheel(amount); }


	// keyboard
	if(IsKeyPressed(KEY_LEFT_CONTROL)){
		pressG();
	}

	int c = 0;
	while((c = GetCharPressed())){
		inputCharacter(c);
	}

}
