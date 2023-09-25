#include <stdio.h>
#include <string.h>
#include <ctype.h>
/*
#include <item.h>

static struct LooseItem looseItems[MAX_LOOSE_ITEMS];
static struct looseItems_ptr;

void addLooseItem(struct LooseItem item){
	if(looseItems_ptr - looseItems >= MAX_LOOSE_ITEMS) return;
	looseItems[looseItems_ptr++] = item;
}

void deleteLooseItem(struct LooseItem * ptr){
	*ptr = *--looseItems_ptr;
}
*/

void generateSource(void);
void generateHeader(void);
void completeTheForms(void);

const char * filebase;
const char * camelSingular;
const char * camelPlural;
char typename[64];
char capsSingular[64];
char capsPlural[64];

int main(int argc, const char * argv[]){

	if(argc < 5) goto usage;

	enum {SOURCE_MODE, HEADER_MODE}   mode = SOURCE_MODE;
	if(strcmp(argv[1], "-c")==0)      mode = SOURCE_MODE;
	else if(strcmp(argv[1], "-h")==0) mode = HEADER_MODE;
	else goto usage;

	filebase = argv[2];
	camelSingular = argv[3];
	camelPlural = argv[4];

	completeTheForms();

	if(strcmp(camelSingular, camelPlural) == 0){
		fprintf(stderr, "it will be bad if singular is the same as plural\n");
		return 1;
	}

	//fprintf(stderr, "filebase = %s\n", filebase);
	//fprintf(stderr, "camelSingular = %s\n", camelSingular);
	//fprintf(stderr, "camelPlural = %s\n", camelPlural);
	//fprintf(stderr, "typename = %s\n", typename);
	//fprintf(stderr, "capsSingular = %s\n", capsSingular);
	//fprintf(stderr, "capsPlural = %s\n", capsPlural);

	if(mode == SOURCE_MODE) generateSource();
	if(mode == HEADER_MODE) generateHeader();

	return 0;

usage:
	printf("spawner -c/-h filebase camelSingular camelPlural\n");
	return 0;

}

void generateHeader(){
	printf("struct %s {\n", typename);
	printf("	// fields\n");
	printf("};\n\n");

	printf("#define MAX_%s 1024\n", capsPlural);
	printf("extern struct %s %s[MAX_%s];\n", typename, camelPlural, capsPlural);
	printf("extern struct %s * %s_ptr;\n\n", typename, camelPlural);

	printf("void add%s(struct %s * %s);\n", typename, typename, camelSingular);
	printf("void erase%s(struct %s * %s);\n", typename, typename, camelSingular);
}

void generateSource(){
	printf("#include <%s.h>\n\n", filebase);

	printf("struct %s %s[MAX_%s];\n", typename, camelPlural, capsPlural);
	printf("struct %s * %s_ptr = %s;\n", typename, camelPlural, camelPlural);
	printf("struct %s * %s_end = %s + MAX_%s;\n\n", typename, camelPlural, camelPlural, capsPlural);

	printf("void add%s(struct %s * p){\n", typename, typename);
	printf("	if(%s_ptr == %s_end) return;\n", camelPlural, camelPlural);
	printf("	*%s_ptr++ = *p;\n", camelPlural);
	printf("}\n\n");

	printf("void erase%s(struct %s * p){\n", typename, typename);
	printf("	if(%s_ptr == %s) return;\n", camelPlural, camelPlural);
	printf("	*p = *--%s_ptr;\n", camelPlural);
	printf("}\n");
}


void capitalize(char C, const char *src, char * dst){
	dst[0] = C;

	int pen = 1;

	for(int i = 1; src[i]; i++){
		if(isupper(src[i])){
			dst[pen++] = '_';
			dst[pen++] = src[i];
		}
		else{
			dst[pen++] = toupper(src[i]);
		}
	}

	dst[pen] = 0;
}

void completeTheForms(){
	char C = toupper(camelSingular[0]);

	strcpy(typename, camelSingular);
	typename[0] = C;

	capitalize(C, camelSingular, capsSingular);
	capitalize(C, camelPlural, capsPlural);
}
