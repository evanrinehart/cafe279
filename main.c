/* MAIN.C */

#include <stdlib.h>
#include <stdio.h>
#include <renderer.h>
#include <loader.h>
#include <engine.h>
#include <bsod.h>

struct Engine engine;

#define WORKSPACE "workspace.save"

int main(int argc, char* argv[]){

	engine.frameNumber = 0;

	int width  = 1920 / 2;
	int height = 1080 / 2;

	int status;

	status = loadConfig(stderr, "config.db");         if(status < 0){ exit(1); }
	status = initializeWindow(width, height, "GAME"); if(status < 0){ exit(1); }
	status = loadAssets();                            if(status < 0){ exit(1); }
	status = loadWorkspace(stderr, WORKSPACE);        if(status < 0){ exit(1); }

	initializeEverything();

	for(;;){
		rerenderEverything();
		dispatchInput();
		// do physics here
		if(windowShouldClose()) break;
		engine.frameNumber++;
	}

	status = saveWorkspace(stderr, WORKSPACE);
	if(status < 0){
		bsod("You realize saveWorkspace failed");
	}

	shutdownEverything();

	return 0;

}
