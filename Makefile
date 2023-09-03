EXE_NAME = game

RAYLIB_VERSION = 4.5.0
RAYLIB_PATH = vendor/raylib/src
RAYLIB_URL = https://github.com/raysan5/raylib.git

SQLITE_NAME = sqlite-amalgamation-3430000
SQLITE_PATH = vendor/sqlite3/$(SQLITE_NAME)
SQLITE_URL = https://www.sqlite.org/2023/$(SQLITE_NAME).zip

CC = gcc

CFLAGS = \
	-g \
	-O2 \
	-I. \
	-Wall \
	-Werror \
	-Wno-error=unused-variable \
	-Wno-error=unused-function \
	-Wno-error=unused-but-set-variable

LIBS = -lGL -lm -lpthread -lrt -lX11

OBJECTS = \
	linear.o \
	floodfill.o \
	bresenham.o \
	symbols.o \
	grid.o \
	doodad.o

$(EXE_NAME) : $(OBJECTS) main.o sqlite3.o libraylib.a
	gcc -o $(EXE_NAME) $(OBJECTS) main.o sqlite3.o libraylib.a $(LIBS)

# implicit rules and compile action for .c files used here
main.o : sqlite3.h rlgl.h raylib.h floodfill.h linear.h grid.h
linear.o : linear.h
bresenham.o : bresenham.h
floodfill.o : floodfill.h
doodad.o : raylib.h symbols.h linear.h doodad.h
grid.o : grid.h raylib.h linear.h


symbols.o : symbols.c
	$(CC) -c $(CFLAGS) symbols.c

symbols.c symbols.h &: tools/symgen symbols.def
	tools/symgen symbols.def

tools/symgen : symgen.c
	gcc -Wall -Werror -o tools/symgen symgen.c

libraylib.a : vendor/raylib
	$(MAKE) -C vendor/raylib/src
	cp vendor/raylib/src/libraylib.a .

raylib.h rlgl.h &: vendor/raylib
	cp vendor/raylib/src/raylib.h .
	cp vendor/raylib/src/rlgl.h .

vendor/raylib :
	mkdir -p vendor
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) vendor/raylib

vendor/sqlite3 :
	rm -rf vendor/sqlite3
	mkdir -p vendor/sqlite3
	wget -P vendor/sqlite3/ $(SQLITE_URL)
	unzip vendor/sqlite3/$(SQLITE_NAME).zip -d vendor/sqlite3/

sqlite3.h sqlite3.c &: vendor/sqlite3
	cp vendor/sqlite3/$(SQLITE_NAME)/sqlite3.c .
	cp vendor/sqlite3/$(SQLITE_NAME)/sqlite3.h .

sqlite3.o : sqlite3.c
	$(CC) -c -Wall sqlite3.c


clean :
	rm -f $(EXE_NAME) main.o $(OBJECTS)
	rm -f tools/symgen
	rm -f symbols.c symbols.h

distclean : clean
	rm -f raylib.h rlgl.h libraylib.a
	rm -f sqlite3.o sqlite3.c sqlite3.h
	rm -rf vendor/sqlite3/
	rm -rf vendor/raylib/
