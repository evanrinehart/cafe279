CC = gcc
DC = gdc

CFLAGS = \
	-g \
	-O2 \
	-I. \
	-Iinclude \
	-Wall \
	-Wextra \
	-Werror \
	-pedantic \
	-Wno-error=unused-variable \
	-Wno-error=unused-parameter \
	-Wno-error=unused-function \
	-Wno-error=unused-but-set-variable

LIBS = -lm -lrt -lpthread -ldl

OBJECTS = \
	main.o \
	brain.o \
	linear.o \
	loader.o \
	clocks.o \
	physics.o \
	floodfill.o \
	bresenham.o \
	chunk.o \
	network.o \
	misc.o \
	messages.o \
	doodad.o

EXE_NAME = game

$(EXE_NAME) : $(OBJECTS) renderer.o sound.o sqlite3.o libraylib.a libenet.a
	gcc -o $(EXE_NAME) $(OBJECTS) renderer.o sound.o sqlite3.o libraylib.a libenet.a $(LIBS) -lGL -lX11

$(EXE_NAME)-nogfx : $(OBJECTS) nullrenderer.o nullsound.o sqlite3.o libenet.a
	gcc -o $(EXE_NAME)-nogfx $(OBJECTS) nullrenderer.o nullsound.o sqlite3.o libenet.a $(LIBS)

# implicit rules and compile action for .c files used here
main.o : engine.h renderer.h brain.h physics.h loader.h bsod.h clocks.h
renderer.o : raylib.h brain.h sound.h renderer.h linear.h bsod.h doodad.h chunk.h megaman.h
nullrenderer.o :
sound.o : raylib.h sound.h
nullsound.o : sound.h
physics.o : doodad.h chunk.h linear.h misc.h
loader.o : loader.h linear.h doodad.h chunk.h sqlite3.h
doodad.o : doodad.h linear.h
chunk.o : chunk.h floodfill.h
linear.o : linear.h
clocks.o : clocks.h
bresenham.o : bresenham.h
floodfill.o : floodfill.h
messages.o : messages.h
misc.o : misc.h
network.o : include/enet/enet.h network.h
brain.o : linear.h bsod.h doodad.h chunk.h megaman.h network.h misc.h

# define a custom pattern rule to compile D files in the same way C files are
#%.o : %.d ; $(DC) -fno-druntime -c $<

RAYLIB_VERSION = master
RAYLIB_PATH = vendor/raylib/src
RAYLIB_URL = https://github.com/raysan5/raylib.git

vendor/raylib :
	mkdir -p vendor
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) vendor/raylib
	sed -i 's/SwapScreenBuffer();/\/\/SwapScreenBuffer();/' vendor/raylib/src/rcore.c
	sed -i 's/PollInputEvents();/\/\/PollInputEvents();/' vendor/raylib/src/rcore.c

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
	$(CC) -c -Wall -Wextra -pedantic sqlite3.c


vendor/enet :
	mkdir -p vendor
	git clone --depth=1 --branch=v1.3.17 https://github.com/lsalzman/enet.git vendor/enet/
	cd vendor/enet/; autoreconf -vfi; ./configure && make

libenet.a include/enet/enet.h &: vendor/enet
	mkdir -p include
	cp -r vendor/enet/include/enet include/
	cp vendor/enet/.libs/libenet.a .

clean :
	rm -f $(EXE_NAME) $(EXE_NAME)-nogfx
	rm -f $(OBJECTS)
	rm -f renderer.o nullrenderer.o sound.o nullsound.o
	rm -f raylib.h rlgl.h libraylib.a
	rm -f libenet.a
	rm -rf include/enet/

distclean : clean
	rm -f sqlite3.c sqlite3.h
	rm -f sqlite3.o
	rm -rf vendor/sqlite3/
	rm -rf vendor/raylib/
	rm -rf vendor/enet/
