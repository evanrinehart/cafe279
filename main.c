/* MAIN.C */

#include <stdlib.h>
#include <renderer.h>
#include <loader.h>
#include <engine.h>

struct Engine engine;

#define WORKSPACE "workspace.save"

int main(int argc, char* argv[]){

	engine.frameNumber = 0;

	int width  = 1920 / 2;
	int height = 1080 / 2;

	int status;

	status = loadConfig("config.db");                 if(status < 0){ exit(1); }
	status = initializeWindow(width, height, "GAME"); if(status < 0){ exit(1); }
	status = loadAssets();                            if(status < 0){ exit(1); }
	status = loadWorkspace(WORKSPACE);                if(status < 0){ exit(1); }

	initializeEverything();

	for(;;){
		rerenderEverything();
		dispatchInput();
		// do physics
		if(windowShouldClose()) break;
		engine.frameNumber++;
	}

	saveWorkspace(WORKSPACE);

	shutdownEverything();

	return 0;

}
