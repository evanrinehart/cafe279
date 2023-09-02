#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <linear.h>

#include <raylib.h>

#define SCALE 16.0
#define REDUCE(x) (floor((x + SCALE/2) / SCALE))
#define SWAP(x,y) do { int temp = i; j = i; i = temp; } while(0)
#define CRASH(X) do {fprintf(stderr, "%s(%d) " X "\n", __FILE__, __LINE__); exit(1);} while(0)

enum bgstatus_t {
	FASTPATH,
	MAINLOOP,
	DONE
};

/* algorithm states, all together
- in a column waiting to output current cell.
- that's always what state its in. So how do you move between states.

- first, output the current cell.
- then three things may happens:
	- stay in column and move up
	- no more cells in this column, and it's the last column
	- no more cells in this column, move to next column
- moving to the next column updates the Y intercepts, which requires a delta X
- delta X is different for the last column
*/

static int status;

static int swapaxis = 0;

static int i;
static int j;
static int lasti;
static int lastj;
static double slope;

static double y;
static double x;
static double waterline;
static int more_columns;
static int elevating;
static int istep;
static double xstep;
static double last_mile;

//static int ultimateKludge = 0;
//static double bx;
//static double by;


void setupSlopeUnderOne(vec2 a, vec2 b, double slope){
	i = REDUCE(a.x);
	j = REDUCE(a.y);
	lasti = REDUCE(b.x);
	lastj = REDUCE(b.y);

	int west = i < lasti;

	elevating = j < lastj;
	istep = west ? 1 : -1;
	xstep = west ? SCALE : -SCALE;

	double lastgrid = lasti*SCALE - (west ? 8 : -8);
	last_mile = b.x - lastgrid; /* negative when going left */
	//printf("b.x=%lf lastgrid=%lf last_mile=%lf\n", b.x, lastgrid, last_mile);

	x = i*SCALE + (west ? 8.0 : -8.0);
	y = a.y + (x - a.x) * slope;
	waterline = j*SCALE + (elevating ? 8.0 : -8.0);
	more_columns = abs(lasti - i);


	// on every step but the last, x increments by SCALE or -SCALE
}

int checkFast(vec2 a, vec2 b){
	i = REDUCE(a.x);
	j = REDUCE(a.y);
	lasti = REDUCE(b.x);
	lastj = REDUCE(b.y);
	
	if(i==lasti && j==lastj){
		status = FASTPATH;
		return 1;
	}
	else{
		return 0;
	}
}

// set up a in-order traversal of the cells covered by line a b
void enumerateCoveredCells(vec2 a, vec2 b){
	if(checkFast(a,b)) return;

	double deltaX = b.x - a.x;
	double deltaY = b.y - a.y;

	slope = deltaY / deltaX;

	if(fabs(slope) > 1.0){
		swapaxis = 1;
		slope = 1.0 / slope;
		vec2 reflectA = {a.y, a.x};
		vec2 reflectB = {b.y, b.x};
		setupSlopeUnderOne(reflectA, reflectB, slope);
		swapaxis = 1;
	}
	else{
		setupSlopeUnderOne(a,b,slope);
		swapaxis = 0;
	}

	status = MAINLOOP;
}


//void draw_vertical_guideline(double x, Color c);
//void draw_horizontal_guideline(double y, Color c);

// return the next cell of a traversal in i j, or 0 if no more cells
int nextCoveredCell(int *iout, int *jout){

	//draw_horizontal_guideline(y, BLUE);

	switch(status){
		case FASTPATH: 
			//printf("FASTPATH\n");
			*iout = i;
			*jout = j;
			status = DONE;
			return 1;
		case DONE:
			return 0;
		case MAINLOOP:
			/* output the cell we're currently on */
			if(swapaxis){
				*iout = j;
				*jout = i;
			}
			else{
				*iout = i;
				*jout = j;
			}

			//printf("y=%lf waterline=%lf\n", y, waterline);

			if(elevating){

				if(y < waterline){ /* move right unless we're out of horizontal steps */
					if(more_columns == 0){ status = DONE; return 1; }
					if(more_columns == 1) xstep = last_mile;
					more_columns--;
					i += istep;
					y += xstep * slope;
				}
				else if(y >= waterline){ /* move up */
					j++;
					waterline += SCALE;
				}
				else{
					if(more_columns == 0){ status = DONE; return 1; }
					if(more_columns == 1) xstep = last_mile;
					more_columns--;
					i += istep;
					y += xstep * slope;

					j++;
					waterline += SCALE;
				}


			}
			else {

				if(y > waterline){ 
					if(more_columns == 0){ status = DONE; return 1; }
					if(more_columns == 1) xstep = last_mile;
					more_columns--;
					i += istep;
					y += xstep * slope;
				}
				else if(y <= waterline){ /* move down */
					j--;
					waterline -= SCALE;
				}
				else{
					if(more_columns == 0){ status = DONE; return 1; }
					if(more_columns == 1) xstep = last_mile;
					more_columns--;
					i += istep;
					y += xstep * slope;

					j--;
					waterline -= SCALE;
				}
			}

			return 1;

		default:
			CRASH("nextCell bad status");
	}
}


