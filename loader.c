#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sqlite3.h>

#include <linear.h>
#include <symbols.h>

#include <doodad.h>
#include <megaman.h>
#include <chunk.h>

#include <loader.h>

static char errorMsg[1024] = "";
static char unlinkErrorMsg[1024] = "";
static int errorLine = 0;

int withSqlite3(const char* filename, int (*proc)(sqlite3 *db)){
	int error;
	int status;
	sqlite3 *db;

	error = sqlite3_open(filename, &db);
	if(error){
		errorLine = __LINE__;
		strcpy(errorMsg, sqlite3_errmsg(db));
		status = -1;
		goto cleanup;
	}

	status = proc(db);
	if(status == -1){
		strcpy(errorMsg, sqlite3_errmsg(db));
	}

cleanup:
	error = sqlite3_close(db);
	if(error){
		char number[64];
		sprintf(number, "%d", error);
		printf("sqlite3_close: %s\n", error==SQLITE_BUSY ? "SQLITE_BUSY" : number);
	}

	return status;
}

int execStatement(sqlite3 *db, const char *sql){
	int error = sqlite3_exec(db, sql, NULL, NULL, NULL);
	return error ? -1 : 0;
}

int withStatement(sqlite3 *db, const char *sql, int (*proc)(sqlite3 *db, sqlite3_stmt *stmt)){
	int status, error;
	sqlite3_stmt *stmt;
	error = sqlite3_prepare(db, sql, -1, &stmt, NULL); if(error){ return -1; }
	status = proc(db, stmt);
	sqlite3_finalize(stmt);
	return status;
}

int withStatementEx(void* u, sqlite3 *db, const char *sql, int (*proc)(void* u, sqlite3 *db, sqlite3_stmt *stmt)){
	int status, error;
	sqlite3_stmt *stmt;
	error = sqlite3_prepare(db, sql, -1, &stmt, NULL); if(error){ return -1; }
	status = proc(u, db, stmt);
	sqlite3_finalize(stmt);
	return status;
}

int iterator(void* u, sqlite3 *db, sqlite3_stmt *stmt){
	int (*proc)(sqlite3_stmt *stmt) = u;
	for(;;){
		switch(sqlite3_step(stmt)){
			case SQLITE_ROW:
				proc(stmt);
				break;
			case SQLITE_DONE:
				return 0;
			default:
				strcpy(errorMsg, sqlite3_errmsg(db));
				return -1;
		}
	}
}

int eachRow(sqlite3 *db, const char *sql, int (*proc)(sqlite3_stmt *stmt)){
	return withStatementEx(proc, db, sql, iterator);
}


void printErrorMsg(){
	if(errorMsg[0] == 0) return;
	printf("%s(%d) sqlite3: %s\n", __FILE__, errorLine, errorMsg);
}


int loadBlock(sqlite3_stmt *stmt){
	int i = sqlite3_column_int(stmt,0);
	int j = sqlite3_column_int(stmt,1);
	int tile = sqlite3_column_int(stmt,2);
	if(i < 0 || i > 255) return 0;
	if(j < 0 || j > 255) return 0;
	chunk.block[i][j] = tile;
	return 0;
}


int loadConfig(const char* filename){
	return 0;
}

int loadMegaman(){
	megaman.x = 24;
	megaman.facing = 1;
	return 0;
}

int loadDoodad(sqlite3_stmt *stmt){
	struct Doodad d;
	d.serial_no = sqlite3_column_int(stmt,0);
	d.pos.x = sqlite3_column_double(stmt,1);
	d.pos.y = sqlite3_column_double(stmt,2);
	strncpy(d.label, (const char*)sqlite3_column_text(stmt,3), 64); d.label[63] = '\0';
	d.symbol = parseSymbol((const char*)sqlite3_column_text(stmt,4));
	addDoodad(&d);
	return 0;
}

int loadWorkspaceDb(sqlite3 *db){
	int status;
	status = eachRow(db, "SELECT * FROM doodads;", loadDoodad); if(status < 0){ return -1; }
	status = eachRow(db, "SELECT * FROM blocks;", loadBlock);   if(status < 0){ return -1; }
	return 0;
}

int loadWorkspace(const char* savename){
	errorMsg[0] = '\0';
	unlinkErrorMsg[0] = '\0';
	errorLine = 0;

	loadMegaman();

	int status = withSqlite3(savename, loadWorkspaceDb);
	if(status < 0){
		printErrorMsg();
		return -1;
	}
	return 0;
}


int saveBlocks(sqlite3 *db, sqlite3_stmt* stmt){
	for(int i = 0; i < 256; i++){
	for(int j = 0; j < 256; j++){
		int tile = chunk.block[i][j];
		if(tile == 0) continue;
		sqlite3_reset(stmt);
		sqlite3_bind_int(stmt, 1, i);
		sqlite3_bind_int(stmt, 2, j);
		sqlite3_bind_int(stmt, 3, tile);
		sqlite3_step(stmt);
	}
	}
	return 0;
}

int saveDoodads(sqlite3 *db, sqlite3_stmt* stmt){
	for(struct Doodad *d = doodads; d < doodad_ptr; d++){
		sqlite3_reset(stmt);
		sqlite3_bind_int(stmt,    1, d->serial_no);
		sqlite3_bind_double(stmt, 2, d->pos.x);
		sqlite3_bind_double(stmt, 3, d->pos.y);
		sqlite3_bind_text(stmt,   4, d->label, -1, NULL);
		sqlite3_bind_text(stmt,   5, stringifySymbol(d->symbol), -1, NULL);
		sqlite3_step(stmt);
	}
	return 0;
}

int saveWorkspaceDb(sqlite3 *db){
	int status;
	status = execStatement(db, "CREATE TABLE doodads(serial_no int, pos_x real, pos_y real, label text, symbol text);");
	status = execStatement(db, "CREATE TABLE blocks(i int, j int, tile int);");
	status = execStatement(db, "BEGIN TRANSACTION;");
	status = withStatement(db, "INSERT INTO doodads VALUES (?,?,?,?,?);", saveDoodads);
	status = withStatement(db, "INSERT INTO blocks VALUES (?,?,?);", saveBlocks);
	status = execStatement(db, "COMMIT;");
	return 0;
}

int saveWorkspace(const char* saveName){
	int status, fd;
	char tempName[64] = "tempsave-XXXXXX";

	/* prepare for oncoming errors */
	errorMsg[0] = '\0';
	unlinkErrorMsg[0] = '\0';
	errorLine = 0;

	fd = mkstemp(tempName);                  if(fd     == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
	status = close(fd);                      if(status == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
	status = withSqlite3(tempName, saveWorkspaceDb);

	if(status == -1){
		status = unlink(tempName);           if(status == -1){ strcpy(unlinkErrorMsg, strerror(errno)); }
		return -1;
	}
	else{
		/* if this fails make no attempt to unlink the temp file which might be a viable save */
		status = rename(tempName, saveName); if(status == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
		return 0;
	}
	return 0;
}
