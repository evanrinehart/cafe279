/* MAIN.C */

#include <stdio.h>
#include <string.h>   // strcpy
#include <stdbool.h>

#include <linear.h>
#include <renderer.h> // ...
#include <physics.h>  // physics
#include <loader.h>   // loadConfig, loadWorkspace, saveWorkspace
#include <engine.h>   // (type)
#include <bsod.h>     // bsod, bsodN
#include <clocks.h>   // chron, chronf, setStartTime
#include <network.h>  // pollNetwork

struct Engine engine;

int main(int argc, char* argv[]){

	setStartTime(chron());

	engine.paused = true;
	engine.frameNumber = 0;
	engine.timeOffset = 0;
	engine.localTime = chronf();
	engine.serverTime = engine.localTime;
	engine.multiplayerEnabled = false;
	engine.multiplayerRole = SERVER;
	strcpy(engine.serverHostname, "localhost");
	engine.serverPort = 12345;

	int width  = 1920 / 2;
	int height = 1080 / 2;

	const char workspace[] = "workspace.db";

	int status;

	status = loadConfig(stderr, "config.db");         if(status < 0){ bsodN("loadConfig failed"); }
	status = initializeWindow(width, height, "GAME"); if(status < 0){ bsodN("initializeWindow failed"); }
	status = loadAssets();                            if(status < 0){ bsod("loadAssets failed"); }
	status = loadWorkspace(stderr, workspace);        if(status < 0){ bsod("loadWorkspace failed"); }

	initializeEverything();

	for(;;){
		engine.localTime = chronf();
		engine.serverTime = engine.localTime + engine.timeOffset;

		rerenderEverything();
		dispatchInput();
		pollNetwork();

		if(engine.paused){
		}
		else{
			physics();
		}

		if(windowShouldClose()) break;

		if(engine.paused == 0) engine.frameNumber++;
	}

	status = saveWorkspace(stderr, workspace);
	if(status < 0){
		bsod("(bug) saveWorkspace failed");
	}

	shutdownEverything();

	return 0;

}
