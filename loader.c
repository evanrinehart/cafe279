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

#define RESET() do{ if(sqlite3_reset(stmt)){ errorLine = __LINE__; \
	errorMsg = sqlite3_erromsg(db); \
	sqlite3_finalize(stmt); \
	return 1; \
} } while (0);

#define BIND(TY, N, X) do{ if(sqlite3_bind_TY(stmt, N, X)){ errorLine = __LINE__; \
	errorMsg = sqlite3_erromsg(db); \
	sqlite3_finalize(stmt); \
	return 1; \
} } while (0);

#define STEP() do{ if(sqlite3_step(stmt) != SQLITE_DONE){ errorLine = __LINE__; \
	errorMsg = sqlite3_erromsg(db); \
	sqlite3_finalize(stmt); \
	return 1; \
} } while (0);

#define PREPARE(X) sqlite3_stmt *stmt; do{ if(sqlite3_prepare(db, X, -1, &stmt, NULL)){ errorLine = __LINE__; \
	errorMsg = sqlite3_erromsg(db); \
	return 1; \
} } while (0);


void printErrorMsg(){
	if(errorMsg[0] == 0) return;
	printf("%s(%d) sqlite3: %s\n", __FILE__, errorLine, errorMsg);
}


int doodadBind(sqlite3_stmt *stmt, struct Doodad *d){
	sqlite3_bind_int(stmt, 1, d->serial_no);
	sqlite3_bind_double(stmt, 2, d->pos.x);
	sqlite3_bind_double(stmt, 3, d->pos.y);
	sqlite3_bind_text(stmt, 4, d->label, -1, NULL);
	sqlite3_bind_text(stmt, 5, stringifySymbol(d->symbol), -1, NULL);
	printf("doodadBind incomplete\n");
	exit(1);
}


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

void loadPiece(int a, int b, int c, int d){
	if(a+128 < 0) return;
	if(b+128 < 0) return;
	printf("load piece %d %d = %d\n", a, b, d);
	chunk.block[a+128][b+128] = d;
}

int restorePiece(sqlite3_stmt *stmt){
	loadPiece(
		sqlite3_column_int(stmt,0),
		sqlite3_column_int(stmt,1),
		sqlite3_column_int(stmt,2),
		sqlite3_column_int(stmt,3)
	);
	return 0;
}

int restoreEverything(sqlite3 *db){
	return 0;
}

int restoreEverythingFrom(const char *saveName){
	return withSqlite3(saveName, restoreEverything);
}

/*




	sqlite3_stmt *stmt2;
	if(sqlite3_prepare(db, "SELECT * FROM config WHERE key = ?", -1, &stmt2, NULL)){ status = 201; goto cleanupStmt; }
	db_config.stats_pos_x = loadConfigF(db, stmt2, "stats_pos_x");
	db_config.stats_pos_y = loadConfigF(db, stmt2, "stats_pos_y");
	sqlite3_finalize(stmt2);

	cleanupStmt:
	sqlite3_finalize(stmt);
	cleanupDb:
	while(sqlite3_close(db) == SQLITE_BUSY){ printf("too busy to close!\n"); sqlite3_sleep(1000); }
	cleanupNothing:

	return 0;
}
*/


int save_pieces(sqlite3 *db, sqlite3_stmt *stmt){
/*
	for(int i = 0; i < db_pieces_ptr; i++){
		struct placed_piece *pp = &db_pieces[i];
		RESET();
		BIND(int, 1, pp->i);
		BIND(int, 2, pp->j);
		BIND(int, 3, pp->layer);
		BIND(int, 4, pp->tile_ix);
		STEP();
	}
*/
	return 0;
}

int save_everything(sqlite3 *db){
	int error;
	error = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL); if(error){ return -1; }
	error = sqlite3_exec(db, "CREATE TABLE placed_pieces(i int, j int, layer int, tile_ex int);", NULL, NULL, NULL); if(error){ return -1; }
	error = sqlite3_exec(db, "CREATE TABLE rooms(id int, air int, volume int);", NULL, NULL, NULL); if(error){ return -1; }
	error = sqlite3_exec(db, "CREATE TABLE config(key TEXT primary key, value ANY);", NULL, NULL, NULL); if(error){ return -1; }

	int status = withStatement(db, "INSERT INTO placed_pieces VALUES (?,?,?,?);", save_pieces); if(status == -1){ return -1; }

/*
	for(int i = 0; i < db_pieces_ptr; i++){
		struct placed_piece *pp = &db_pieces[i];
		if(sqlite3_reset(stmt)){ status = 5; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 1, pp->i)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 2, pp->j)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 3, pp->layer)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 4, pp->tile_ix)){ status = 6; goto cleanupStmt; }
		if(sqlite3_step(stmt) != SQLITE_DONE){ status = 7; goto cleanupStmt; }
	}
*/

	return 0;
}

int saveEverythingTo(const char *saveName){

	int status, fd;
	char tempName[64] = "tempsave-XXXXXX";

	/* prepare for oncoming errors */
	errorMsg[0] = '\0';
	unlinkErrorMsg[0] = '\0';
	errorLine = 0;

	fd = mkstemp(tempName);                  if(fd     == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
	status = close(fd);                      if(status == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
	status = withSqlite3(tempName, save_everything);

	if(status == -1){
		status = unlink(tempName);           if(status == -1){ strcpy(unlinkErrorMsg, strerror(errno)); }
		return -1;
	}
	else{
		/* if this fails make no attempt to unlink the temp file which might be a viable save */
		status = rename(tempName, saveName); if(status == -1){ errorLine = __LINE__; strcpy(errorMsg, strerror(errno)); return -1; }
		return 0;
	}

}



/* DATABASE DUMP AND RESTORE */

/*
int dumpConfigF(sqlite3 *db, sqlite3_stmt *stmt, const char *key, double value){
	// INSERT INTO config VALUES (?,?); 
	if(sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC)) {
		printf("%s line %d bind_text: %s\n", __FILE__, __LINE__, sqlite3_errmsg(db));
		return 101;
	}
	if(sqlite3_bind_double(stmt, 2, value)) return 102;
	if(sqlite3_step(stmt) != SQLITE_DONE) return 103;
	if(sqlite3_reset(stmt)) return 104;
	return 0;
}

double loadConfigF(sqlite3 *db, sqlite3_stmt *stmt, const char *key){
	// SELECT * FROM config WHERE key = ?; 
	double out;
	if(sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC)) { printf("%s line %d bind_text\n", __FILE__, __LINE__); exit(1); }
	if(sqlite3_step(stmt) != SQLITE_ROW){ printf("%s line %d step \n", __FILE__, __LINE__); exit(1); }
	out = sqlite3_column_double(stmt,1);
	if(sqlite3_reset(stmt)) { printf("%s line %d reset\n", __FILE__, __LINE__); exit(1); }
	return out;
}
*/



/*
int dumpRooms(sqlite3 *db){

	PREPARE("INSERT INTO rooms VALUES (?,?,?);");

	for(int i = 2; i < room_counter; i++){
		if(!roomExists(i)) continue;
		struct Room *pp = &db_rooms[i];

		RESET();
		BIND(int, 1, pp->id);
		BIND(int, 2, pp->air);
		BIND(int, 3, pp->volume);
		STEP();

	}

	sqlite3_finalize(stmt);

	return 0;
}

int dumpDatabaseToTemp(const char* tempname){
	int status = 0;
	sqlite3 *db;
	sqlite3_stmt *stmt;

	if(sqlite3_open(tempname, &db)){ status = 1; goto cleanupNothing; }
	if(sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL)){ status = 2; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE placed_pieces(i int, j int, layer int, tile_ex int);", NULL, NULL, NULL)){ status = 3; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE rooms(id int, air int, volume int);", NULL, NULL, NULL)){ status = 33; goto cleanupDb; }
	if(sqlite3_exec(db, "CREATE TABLE config(key TEXT primary key, value ANY);", NULL, NULL, NULL)){ status = 333; goto cleanupDb; }

	if(sqlite3_prepare(db, "INSERT INTO placed_pieces VALUES (?,?,?,?);", -1, &stmt, NULL)){ status = 4; goto cleanupDb; }

	for(int i = 0; i < db_pieces_ptr; i++){
		struct placed_piece *pp = &db_pieces[i];
		if(sqlite3_reset(stmt)){ status = 5; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 1, pp->i)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 2, pp->j)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 3, pp->layer)){ status = 6; goto cleanupStmt; }
		if(sqlite3_bind_int(stmt, 4, pp->tile_ix)){ status = 6; goto cleanupStmt; }
		if(sqlite3_step(stmt) != SQLITE_DONE){ status = 7; goto cleanupStmt; }
	}

	sqlite3_stmt *stmt2;
	if(sqlite3_prepare(db, "INSERT INTO rooms VALUES (?,?,?);", -1, &stmt2, NULL)){ status = 301; goto cleanupStmt; }

	for(int i = 2; i < room_counter; i++){
		if(!roomExists(i)) continue;
		struct Room *pp = &db_rooms[i];
		if(sqlite3_reset(stmt2)){ status = 305; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 1, pp->id)){ status = 306; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 2, pp->air)){ status = 307; goto cleanupStmt2; }
		if(sqlite3_bind_int(stmt2, 3, pp->volume)){ status = 308; goto cleanupStmt2; }
		if(sqlite3_step(stmt2) != SQLITE_DONE){ status = 399; goto cleanupStmt2; }
	}

	sqlite3_stmt *stmt3;
	if(sqlite3_prepare(db, "INSERT INTO config VALUES (?, ?);", -1, &stmt3, NULL)){ status = 202; goto cleanupStmt2; }
	if(status = dumpConfigF(db, stmt3, "stats_pos_x", db_config.stats_pos_x)){ goto cleanupStmt3; }
	if(status = dumpConfigF(db, stmt3, "stats_pos_y", db_config.stats_pos_y)){ goto cleanupStmt3; }

	if(sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL)){ status = 8; goto cleanupStmt3; }

	cleanupStmt3:
	sqlite3_finalize(stmt3);
	cleanupStmt2:
	sqlite3_finalize(stmt2);
	cleanupStmt:
	sqlite3_finalize(stmt);
	cleanupDb:
	while(sqlite3_close(db) == SQLITE_BUSY){ printf("too busy to close!\n"); sqlite3_sleep(1000); }
	cleanupNothing:

	return status;
}
*/

/*
int dumpDatabase(const char* outname){
	int status = 0;
	char tempname[64] = "tempsave-XXXXXX";
	int fd;

	fd = mkstemp(tempname);                // might fail
	close(fd);                             // might fail
	status = dumpDatabaseToTemp(tempname); // might fail
	if(status)
		unlink(tempname);                  // might fail
	else
		rename(tempname, outname);         // might fail

	return status;
}

*/



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
	status = eachRow(db, "SELECT * FROM doodads;", loadDoodad);         if(status < 0){ return -1; }
	status = eachRow(db, "SELECT * FROM placed_pieces;", restorePiece); if(status < 0){ return -1; }
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



int saveWorkspace(const char* filename){
	// TODO
	return 0;
}
