/* MAIN.C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <math.h>

#include <unistd.h>
#include <signal.h>

#include <threads.h>

#include <linear.h>
#include <renderer.h> // ...
#include <physics.h>  // physics
#include <loader.h>   // loadConfig, loadWorkspace, saveWorkspace
#include <engine.h>   // (type)
#include <clocks.h>   // chron, chronf, setStartTime
#include <network.h>  // pollNetwork
#include <bsod.h>     // bsod
#include <misc.h>     // readBool

struct Engine engine;

static mtx_t masterLock;
static thrd_t mainThread;

int mainThreadProc(void* u);
int graphicsThreadProc(void *u);

int main(int argc, char* argv[]){

	setStartTime(chron());

	engine.shouldClose = false;
	engine.paused = true;
	engine.frameNumber = 0;
	engine.timeOffset = 0;
	engine.localTime = chronf();
	engine.serverTime = engine.localTime;
	engine.dedicated = false;
	strcpy(engine.serverHostname, "localhost");
	engine.serverPort = 12345;
	engine.networkStatus = OFFLINE;
	engine.videoEnabled = false;
	engine.vsync = true;
	engine.vsyncHint = true;

	int width  = 1920 / 2;
	int height = 1080 / 2;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0) { engine.dedicated = true; continue; }
		if (strcmp(argv[i], "-p") == 0 && i+1 < argc) { engine.serverPort = atoi(argv[i+1]); i++; continue; }
		if (strcmp(argv[i], "--window") == 0 && i+1 < argc) { sscanf(argv[i+1], "%dx%d", &width, &height); i++; continue; }
		if (strcmp(argv[i], "--vsync") == 0 && i+1 < argc) { engine.vsync = readBool(argv[i+1]); i++; continue; }
		strcpy(engine.serverHostname, argv[i]);
	}

	engine.graphical = engine.dedicated ? false : true;
	engine.vsyncHint = engine.vsync;

	int status;

	if (engine.graphical) {
		status = initializeWindow(width, height, "GAME");    if (status < 0) bsod("NO GRAPHICS");
		status = loadAssets();                               if (status < 0) bsod("NO ASSETS");
	}
	else {
		// rely on SIGINT to end the program without graphical UI
		void sigintHandler(int sig){ engine.shouldClose = true; }
		struct sigaction sa = { .sa_handler = sigintHandler, .sa_flags = 0 };
		sigemptyset(&sa.sa_mask);
		status = sigaction(SIGINT, &sa, NULL);               if (status < 0) bsod("NO SIGINT");
	}

	status = loadWorkspace(stderr, "workspace.db");          if (status < 0) bsod("NO WORKSPACE.DB");
	initializeEverything();
	status = mtx_init(&masterLock, mtx_plain);               if (status < 0) bsod("NO MASTER LOCK");
	status = thrd_create(&mainThread, mainThreadProc, NULL); if (status < 0) bsod("NO THREAD");

	if(engine.graphical) {
		graphicsThreadProc(NULL);
	}
	else {
		status = enableServer();                             if (status < 0) bsod("NO SERVER");
	}

	thrd_join(mainThread, 0);
	mtx_destroy(&masterLock);
	status = saveWorkspace(stderr, "workspace.db");          if (status < 0) bsod("saveWorkspace failed");
	shutdownEverything();

	return 0;

}


int mainThreadProc(void* u){

	int highestUpdateCompleted = -1;
	double updateZeroTime = chronf();

	for (;;) {
		mtx_lock(&masterLock);

		engine.localTime = chronf();
		engine.serverTime = engine.localTime + engine.timeOffset;

		if(engine.inputFresh) { dispatchInput(); engine.inputFresh = false; }

		int updates = floor((engine.localTime - updateZeroTime) * 60.0);
		int missedUpdates = updates - highestUpdateCompleted;

		for(int i = 0; i < missedUpdates; i++){
			physics();
			engine.frameNumber++;
		}

		highestUpdateCompleted = updates;

		mtx_unlock(&masterLock);

		if (engine.shouldClose) return 0;

		if(engine.networkStatus == OFFLINE) usleep(1000);
		else pollNetworkTimeout(3);
	}

}

int graphicsThreadProc(void *u){
	for(;;){
		mtx_lock(&masterLock);
		renderPollInput();
		rerenderEverything();

		manageVsync();

		mtx_unlock(&masterLock);

		if(windowShouldClose()) { engine.shouldClose = 1; return 0; }
		renderSwap();
	}
}
