#define CELL 16.0 // the size of a cell in pixels
#define REDUCE(x) (floor((x + CELL/2) / CELL)) // cell index for world coordinate
#define EXPAND(i) (CELL*(i)) // world coordinate for center of cell at index

Vector2 worldToScreen(vec2 p);
vec2 screenToWorld(Vector2 q);
void getViewBounds(double *l, double *r, double *b, double *t);

void drawVerticalRule(double worldX, Color c);
void drawHorizontalRule(double worldY, Color c);
void drawGridVerticals(double spacing, double offset, Color color);
void drawGridHorizontals(double spacing, double offset, Color color);
void drawGrid(double spacing, double offset, Color color);
void drawSegmentC(vec2 a, vec2 b, Color c);
void drawSegment(vec2 a, vec2 b);
void drawSolidBlock(int i, int j, Color c);
void drawLabel(const char *txt, vec2 p);
void drawBall(vec2 p, double r, Color c);
void drawSprite(Texture tex, vec2 p, int flip);
void drawUISprite(Texture tex, double x, double y, double zoom);
