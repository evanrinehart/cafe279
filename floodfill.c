#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <floodfill.h>

typedef struct {int i; int j;} int2;

#define STACK_SIZE (4 * FFSIZE * FFSIZE)

static int2 stack[STACK_SIZE];
static int2 *stack_ptr = stack;
static int2 *stack_end = stack + STACK_SIZE;

static char flooded[FFSIZE][FFSIZE];
static int occupied = 0;

#define CRASH(MSG) do { \
	fprintf(stderr, "%s(%d) %s\n", __FILE__, __LINE__, MSG); \
	exit(1); \
} while (0)

static void push(int i, int j){
	if(stack_ptr >= stack_end) CRASH("floodfill stack overflow");

	stack_ptr->i = i;
	stack_ptr->j = j;
	stack_ptr++;
}

static int pop(int *iout, int *jout){
	if (stack_ptr == stack) return 0;
	stack_ptr--;
	*iout = stack_ptr->i;
	*jout = stack_ptr->j;
	return 1;
}


/* floodfill 2.0 */
/* this floods a map of edges which cannot be crossed, calling a callback for each i,j flooded. */
/* unlike before there are no observable side effects. Not reentrant. */

int flood(
	int i, int j,
	char horiz[FFSIZE][FFSIZE],
	char vert[FFSIZE][FFSIZE],
	void* userdata,
	int (*visit)(void* userdata, int i, int j)
){
	if(occupied) CRASH("detected nested call to flood");

	int status = 0;

	occupied = 1;
	stack_ptr = stack;
	bzero(flooded, sizeof flooded);

	do {
		if (flooded[i][j]) continue;

		flooded[i][j] = 1;
		status = visit(userdata,i,j); if(status) break;

		if (i < FFSIZE-1 &&  vert[i+1][j] == 0 && !flooded[i+1][j]) push(i+1, j);
		if (i > 0        &&  vert[i][j]   == 0 && !flooded[i-1][j]) push(i-1, j);
		if (j < FFSIZE-1 && horiz[i][j+1] == 0 && !flooded[i][j+1]) push(i, j+1);
		if (j > 0        && horiz[i][j]   == 0 && !flooded[i][j-1]) push(i, j-1);
	} while (pop(&i, &j));

	occupied = 0;

	return status;
}
