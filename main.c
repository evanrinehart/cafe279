/* MAIN.C */

#include <stdlib.h>
#include <stdio.h>
#include <linear.h>
#include <renderer.h>
#include <physics.h>
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

	status = loadConfig(stderr, "config.db");         if(status < 0){ bsodN("loadConfig failed"); }
	status = initializeWindow(width, height, "GAME"); if(status < 0){ bsodN("initializeWindow failed"); }
	status = loadAssets();                            if(status < 0){ bsod("loadAssets failed"); }
	status = loadWorkspace(stderr, WORKSPACE);        if(status < 0){ bsod("loadWorkspace failed"); }

	initializeEverything();

	for(;;){
		rerenderEverything();
		dispatchInput();
		physics();
		if(windowShouldClose()) break;
		engine.frameNumber++;
	}

	status = saveWorkspace(stderr, WORKSPACE);
	if(status < 0){
		bsod("(bug) saveWorkspace failed");
	}

	shutdownEverything();

	return 0;

}
