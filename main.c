/* MAIN.C */

#include <stdlib.h>

#include <engine.h>

// components
#include <renderer.h>
#include <loader.h>



/* GLOBALS */

struct Engine engine = { .frameNumber = 0LL };

int main(int argc, char* argv[]){

	int width  = 1920 / 2;
	int height = 1080 / 2;

	int status;

	status = loadConfig("config.db");                 if(status < 0){ exit(1); }
	status = initializeWindow(width, height, "GAME"); if(status < 0){ exit(1); }
	status = loadAssets();                            if(status < 0){ exit(1); }
	status = loadWorkspace("workspace.save");         if(status < 0){ exit(1); }

	initializeEverything();

	for(;;){
		rerenderEverything();
		dispatchInput();
		// do physics
		if(windowShouldClose()) break;
		engine.frameNumber++;
	}

	//int dumpStatus = dumpDatabase("workspace.save");
	//printf("dumpDatabase returned %d\n", dumpStatus);

	shutdownEverything();

	return 0;

}
