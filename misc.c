#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

void printData(unsigned char buf[], int n){
	for(int i = 0; i < n; i++){
		unsigned char byte = buf[i];
		if(isprint(byte) && byte != ' '){
			printf("%c ", byte);
		}
		else{
			printf("#%u ", byte);
		}
	}

	puts("");
}

double randf(){
	int i = rand();
	double x = i;
	return x / INT_MAX;
}

double randi(int a, int b){
	return a + rand() % (b - a + 1);
}


bool readBool(const char * string){
	if(sscanf(string, "true") > 0) return true;
	if(sscanf(string, "false") > 0) return false;
	return false;
}
