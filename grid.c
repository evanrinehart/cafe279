#include <math.h>
#include <raylib.h>
#include <linear.h>

#include <grid.h>

extern vec2 camera;
extern int screenW;
extern int screenH;
extern double zoom;

Vector2 worldToScreen(vec2 p){
	double x = floor( zoom * p.x + screenW/2 - zoom * camera.x);
	double y = floor(-zoom * p.y + screenH/2 + zoom * camera.y);
	Vector2 v = {x,y};
	return v;
}

vec2 screenToWorld(Vector2 q){
	double x = (q.x + zoom*camera.x - screenW/2) / zoom;
	double y = (q.y - zoom*camera.y - screenH/2) / -zoom;
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

void drawLabel(const char *txt, vec2 p){
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
