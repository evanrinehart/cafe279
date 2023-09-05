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

int initializeWindow(int w, int h, const char* title);
int loadAssets();

void initializeEverything();

void dispatchInput();
void rerenderEverything();

int windowShouldClose();
void shutdownEverything();
