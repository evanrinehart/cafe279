/* interface to the renderer */

bool rendererExists();

int initializeWindow(int w, int h, const char* title);
int loadAssets();

void dispatchInput();
void rerenderEverything();

int windowShouldClose();
void shutdownRenderer();

void renderPollInput();
void renderSwap();

void bsodLoop(const char *);
