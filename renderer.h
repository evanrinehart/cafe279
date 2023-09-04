/* interface to the renderer */

#ifndef RL_COLOR_TYPE
#define RL_COLOR_TYPE

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
    unsigned char r;        // Color red value
    unsigned char g;        // Color green value
    unsigned char b;        // Color blue value
    unsigned char a;        // Color alpha value
} Color;

#endif

#define CELL 16.0 // the size of a cell in pixels
#define REDUCE(x) (floor(((x) + CELL/2) / CELL)) // cell index for world coordinate
#define EXPAND(i) (CELL*(i)) // world coordinate for center of cell at index

void initializeWindow(int w, int h, const char* title);
void loadAssets();

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

void drawDoodad(struct Doodad *d);
void drawMegaman(vec2 p, int flip);

void dispatchInput();

// global structs to be read-only by other modules
extern struct Screen {
	int w;
	int h;
	vec2 mouse;
} screen;

extern struct World {
	vec2 mouse;
} world;



