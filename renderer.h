/* interface to the renderer */

bool rendererExists();

int initializeWindow(int w, int h, const char* title);
int loadAssets();

void initializeEverything();

void dispatchInput();
void rerenderEverything();

int windowShouldClose();
void shutdownEverything();



int enableServer();

void renderPollInput();
void renderSwap();
