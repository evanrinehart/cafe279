#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sqlite3.h>

#include <linear.h>

#include <doodad.h>
#include <megaman.h>
#include <chunk.h>

static FILE* debugLog;

int withSqlite3(const char* filename, int (*proc)(sqlite3 *db));
int withStatementEx(void* u, sqlite3 *db, const char *sql, int (*proc)(void* u, sqlite3 *db, sqlite3_stmt *stmt));
int withStatement(sqlite3 *db, const char *sql, int (*proc)(sqlite3 *db, sqlite3_stmt *stmt));
int withEachRow(sqlite3 *db, const char *sql, int (*proc)(sqlite3_stmt *stmt));

/* Sqlite3 utility routines */

#define PRINT_SQL_ERROR(DB) do { fprintf(debugLog, "%s(%d) sqlite3: %s\n", __FILE__, __LINE__, sqlite3_errmsg(DB)); } while (0)
#define PRINT_NOTE(MSG) do { fprintf(debugLog, "%s(%d) %s\n", __FILE__, __LINE__, MSG); } while (0)
#define PRINT_ERRNO(NAME) do { fprintf(debugLog, "%s(%d) %s: %s\n", __FILE__, __LINE__, NAME, strerror(errno)); } while (0)

#define TRY_EXEC(DB, SQL) \
	do { \
		PRINT_NOTE(SQL); \
		int error = sqlite3_exec(DB, SQL, NULL, NULL, NULL); \
		if(error){ PRINT_SQL_ERROR(DB); return -1; } \
	} while (0)

#define TRY_STMT(DB, SQL, PROC) \
	do { \
		PRINT_NOTE(SQL); \
		int status = withStatement(DB, SQL, PROC); \
		if(status < 0){ return -1; } \
	} while (0)


#define TRY_EACHROW(DB, SQL, PROC) \
	do { \
		PRINT_NOTE(SQL); \
		int status = withEachRow(DB, SQL, PROC); \
		if(status < 0){ return -1; } \
	} while (0)

int withSqlite3(const char* filename, int (*proc)(sqlite3 *db)){
	int status;

	sqlite3 *db;
	int error = sqlite3_open(filename, &db);
	if(error){
		PRINT_SQL_ERROR(db);
		status = -1;
		goto cleanup;
	}

	status = proc(db);

cleanup:
	error = sqlite3_close(db);
	if(error){
		#define SHOW_BUSY(ERR) ((ERR)==SQLITE_BUSY ? " (SQLITE_BUSY)" : "")
		fprintf(debugLog, "%s(%d) sqlite3_close returned %d%s\n", __FILE__, __LINE__, error, SHOW_BUSY(error));
	}
	return status;
}

typedef void (*funcPtr)(void);

int withStatementExF(funcPtr u, sqlite3 *db, const char *sql, int (*proc)(funcPtr u, sqlite3 *db, sqlite3_stmt *stmt)){
	int status, error;
	sqlite3_stmt *stmt;
	error = sqlite3_prepare(db, sql, -1, &stmt, NULL);
	if(error){
		PRINT_SQL_ERROR(db);
		return -1;
	}
	status = proc(u, db, stmt);
	sqlite3_finalize(stmt);
	return status;
}

int withStatementEx(void* u, sqlite3 *db, const char *sql, int (*proc)(void* u, sqlite3 *db, sqlite3_stmt *stmt)){
	int status, error;
	sqlite3_stmt *stmt;
	error = sqlite3_prepare(db, sql, -1, &stmt, NULL);
	if(error){
		PRINT_SQL_ERROR(db);
		return -1;
	}
	status = proc(u, db, stmt);
	sqlite3_finalize(stmt);
	return status;
}


int simpleWrapper(funcPtr u, sqlite3 *db, sqlite3_stmt *stmt){
	int (*proc)(sqlite3 *db, sqlite3_stmt *stmt) = (int (*)(sqlite3 *, sqlite3_stmt *))u;
	return proc(db, stmt);
}

int withStatement(sqlite3 *db, const char *sql, int (*proc)(sqlite3 *db, sqlite3_stmt *stmt)){
	return withStatementExF((funcPtr)proc, db, sql, simpleWrapper);
}

int iterator(funcPtr u, sqlite3 *db, sqlite3_stmt *stmt){
	int (*proc)(sqlite3_stmt *stmt) = (int (*)(sqlite3_stmt *))u;
	for(;;){
		switch(sqlite3_step(stmt)){
			case SQLITE_ROW:
				proc(stmt);
				break;
			case SQLITE_DONE:
				return 0;
			default:
				PRINT_SQL_ERROR(db);
				return -1;
		}
	}
}

int withEachRow(sqlite3 *db, const char *sql, int (*proc)(sqlite3_stmt *stmt)){
	return withStatementExF((funcPtr)proc, db, sql, iterator);
}



/* Concrete loaders and savers */

int loadConfig(FILE* logfile, const char* filename){
	debugLog = logfile;
	PRINT_NOTE("loadConfig...");
	return 0;
}

int loadMegaman(){
	megaman.x = -6 * 16;
	megaman.facing = 1;
	return 0;
}

int loadRoom(sqlite3_stmt *stmt){
	int rid    = sqlite3_column_int(stmt,0);
	int air    = sqlite3_column_int(stmt,1);
	int volume = 0; // measure the volume later
	//int volume = sqlite3_column_int(stmt,2);
	addRoom(rid, air, volume);
	return 0;
}

int loadRoomCell(sqlite3_stmt *stmt){
	int rid  = sqlite3_column_int(stmt,0);
	int i    = sqlite3_column_int(stmt,1);
	int j    = sqlite3_column_int(stmt,2);
	chunk.room[i][j] = rid;
	return 0;
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

int saveRooms(sqlite3 *db, sqlite3_stmt* stmt){
	for(struct Room *r = rooms; r < rooms_ptr; r++){
		if(r->id == 0) continue;
		if(r->id == 1) continue;
		if(!roomExists(r->id, NULL, NULL)) continue;
		sqlite3_reset(stmt);
		sqlite3_bind_int(stmt, 1, r->id);
		sqlite3_bind_int(stmt, 2, r->air);
		//sqlite3_bind_int(stmt, 3, r->volume);
		sqlite3_step(stmt);
	}
	return 0;
}

int saveRoomCells(sqlite3 *db, sqlite3_stmt* stmt){
	for(int i = 0; i < 256; i++){
	for(int j = 0; j < 256; j++){
		int rid = chunk.room[i][j];
		if(rid == 0 || rid == 1) continue;
		sqlite3_reset(stmt);
		sqlite3_bind_int(stmt, 1, rid);
		sqlite3_bind_int(stmt, 2, i);
		sqlite3_bind_int(stmt, 3, j);
		sqlite3_step(stmt);
	}}
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

int saveDoodads(sqlite3 *db, sqlite3_stmt* stmt){
	for(struct Doodad *d = doodads; d < doodads_ptr; d++){
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


int loadWorkspaceDb(sqlite3 *db){
	TRY_EACHROW(db, "SELECT * FROM doodads;", loadDoodad);
	TRY_EACHROW(db, "SELECT * FROM blocks;", loadBlock);
	TRY_EACHROW(db, "SELECT * FROM rooms;", loadRoom);
	TRY_EACHROW(db, "SELECT * FROM room_cells;", loadRoomCell);
	return 0;
}

int loadWorkspace(FILE* logfile, const char* savename){
	debugLog = logfile;

	PRINT_NOTE("loadWorkspace...");

	loadMegaman();

	int status = withSqlite3(savename, loadWorkspaceDb); if(status < 0){ return -1; }

	return 0;
}

int saveWorkspaceToDb(sqlite3 *db){
	TRY_EXEC(db, "CREATE TABLE doodads(serial_no int, pos_x real, pos_y real, label text, symbol text);");
	TRY_EXEC(db, "CREATE TABLE blocks(i int, j int, tile int);");
	TRY_EXEC(db, "CREATE TABLE rooms(id int, air int);");
	TRY_EXEC(db, "CREATE TABLE room_cells(id int, i int, j int);");
	TRY_EXEC(db, "BEGIN TRANSACTION;");

	TRY_STMT(db, "INSERT INTO doodads VALUES (?,?,?,?,?);", saveDoodads);
	TRY_STMT(db, "INSERT INTO blocks VALUES (?,?,?);", saveBlocks);
	TRY_STMT(db, "INSERT INTO rooms VALUES (?,?);", saveRooms);
	TRY_STMT(db, "INSERT INTO room_cells VALUES (?,?,?);", saveRoomCells);

	TRY_EXEC(db, "COMMIT;");
	return 0;
}

int saveWorkspaceWorker(const char* saveName){
	int status, fd;
	char tempName[64] = "tempsave-XXXXXX";

	fd = mkstemp(tempName);                  if(fd     == -1){ PRINT_ERRNO("mkstemp"); return -1; }
	status = close(fd);                      if(status == -1){ PRINT_ERRNO("close"); return -1; }
	status = withSqlite3(tempName, saveWorkspaceToDb);

	if(status == -1){
		status = unlink(tempName);           if(status == -1){ PRINT_ERRNO("unlink"); return -1; }
		return -1;
	}
	else{
		/* If this fails, then make no attempt to unlink the temp file. It might be a viable save */
		status = rename(tempName, saveName); if(status == -1){ PRINT_ERRNO("rename"); return -1; }
		return 0;
	}
	return 0;
}

int saveWorkspace(FILE* logfile, const char* saveName){
	debugLog = logfile;
	PRINT_NOTE("saveWorkspace...");
	return saveWorkspaceWorker(saveName);
}


