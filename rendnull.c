/*

	null renderer

	It has the same API as the renderer but has no functionality.

	So a dedicated server can be built on an actual server without graphics.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

bool rendererExists() {
	return false;
}

void shutdownRenderer() {
}

int initializeWindow(int w, int h, const char* title){
	return 0;
}

int windowShouldClose(){
	return 0;
}

int loadAssets(){
	return 0;
}

void dispatchInput(){
}

void renderSwap(){
}

void renderPollInput(){
}

void rerenderEverything(){
}

void bsod(const char* finalMsg){
	fprintf(stderr, "Final message: %s\n", finalMsg);
	exit(1);
}
