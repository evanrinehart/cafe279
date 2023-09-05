#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <floodfill.h>


static int istack[FFSIZE * FFSIZE];
static int jstack[FFSIZE * FFSIZE];
static int stack_ptr = 0;
static int stack_ptr_max = FFSIZE * FFSIZE;

static void push(int i, int j){
	if(stack_ptr == stack_ptr_max){ puts("floodfill stack overflow\n"); exit(1); }
	istack[stack_ptr] = i;
	jstack[stack_ptr] = j;
	stack_ptr++;
}

static int pop(int *i, int *j){
	if(stack_ptr == 0) return 0;
	stack_ptr--;
	*i = istack[stack_ptr];
	*j = jstack[stack_ptr];
	return 1;
}


/* floodfill 2.0 */
/* this floods a map of edges which cannot be crossed, calling a callback for each i,j flooded. */
/* unlike before there are no observable side effects. Not reentrant. */

static char flooded[FFSIZE][FFSIZE];
static int occupied = 0;

int flood(int i, int j, char horiz[FFSIZE][FFSIZE], char vert[FFSIZE][FFSIZE], void* userdata, int (*visit)(void* userdata, int i, int j)){

	int status = 0;

	if(occupied){
		printf("(bug) flood is not re-entrant sorry\n");
		exit(1);
	}

	occupied = 1;

	bzero(flooded, FFSIZE * FFSIZE);

	stack_ptr = 0;

	do {
		if(flooded[i][j]) continue;

		flooded[i][j] = 1;
		if((status = visit(userdata,i,j))) break;
		

		if(i < FFSIZE-1 &&  vert[i+1][j] == 0 && !flooded[i+1][j]) push(i+1, j);
		if(i > 0        &&  vert[i][j]   == 0 && !flooded[i-1][j]) push(i-1, j);
		if(j < FFSIZE-1 && horiz[i][j+1] == 0 && !flooded[i][j+1]) push(i, j+1);
		if(j > 0        && horiz[i][j]   == 0 && !flooded[i][j-1]) push(i, j-1);
	} while (pop(&i, &j));

	occupied = 0;

	return status;
}
