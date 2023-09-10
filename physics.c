#include <symbols.h>
#include <linear.h>
#include <doodad.h>

/* run physics for 1 step */
void physics(){

	for(struct Doodad *d = doodads; d < doodad_ptr; d++){
		updateDoodad(d);
	}

}
