CC = gcc
DC = gdc

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
	loader.o \
	clocks.o \
	renderer.o \
	physics.o \
	floodfill.o \
	bresenham.o \
	chunk.o \
	doodad.o

EXE_NAME = game
$(EXE_NAME) : $(OBJECTS) main.o sqlite3.o libraylib.a
	gcc -o $(EXE_NAME) $(OBJECTS) main.o sqlite3.o libraylib.a $(LIBS)

# implicit rules and compile action for .c files used here
main.o : engine.h renderer.h physics.h loader.h bsod.h clocks.h
renderer.o : raylib.h renderer.h linear.h bsod.h doodad.h chunk.h megaman.h
physics.o : doodad.h chunk.h linear.h
loader.o : loader.h linear.h doodad.h chunk.h sqlite3.h
doodad.o : doodad.h linear.h
chunk.o : chunk.h floodfill.h
linear.o : linear.h
clocks.o : clocks.h
bresenham.o : bresenham.h
floodfill.o : floodfill.h

# define a custom pattern rule to compile D files in the same way C files are
#%.o : %.d ; $(DC) -fno-druntime -c $<

RAYLIB_VERSION = master
RAYLIB_PATH = vendor/raylib/src
RAYLIB_URL = https://github.com/raysan5/raylib.git

vendor/raylib :
	mkdir -p vendor
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) vendor/raylib

libraylib.a : vendor/raylib
	$(MAKE) -C vendor/raylib/src
	cp vendor/raylib/src/libraylib.a .

raylib.h rlgl.h &: vendor/raylib
	cp vendor/raylib/src/raylib.h .
	cp vendor/raylib/src/rlgl.h .

SQLITE_NAME = sqlite-amalgamation-3430000
SQLITE_PATH = vendor/sqlite3/$(SQLITE_NAME)
SQLITE_URL = https://www.sqlite.org/2023/$(SQLITE_NAME).zip

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
	rm -f raylib.h rlgl.h libraylib.a

distclean : clean
	rm -f sqlite3.c sqlite3.h
	rm -f sqlite3.o
	rm -rf vendor/sqlite3/
	rm -rf vendor/raylib/
