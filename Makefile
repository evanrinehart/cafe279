CC = gcc
DC = gdc

CFLAGS = \
	-g \
	-O2 \
	-Iinclude \
	-Wall \
	-Wextra \
	-Werror \
	-pedantic \
	-Wno-error=unused-variable \
	-Wno-unused-parameter \
	-Wno-unused-function \
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
	chunk.o \
	network.o \
	misc.o \
	messages.o \
	sync.o \
	doodad.o

EXE_NAME = game

$(EXE_NAME) : $(OBJECTS) renderer.o sound.o sqlite3.o libraylib.a libenet.a
	gcc -o $(EXE_NAME) $(OBJECTS) renderer.o sound.o sqlite3.o libraylib.a libenet.a $(LIBS) -lGL -lX11

$(EXE_NAME)-nogfx : $(OBJECTS) nullrenderer.o nullsound.o sqlite3.o libenet.a
	gcc -o $(EXE_NAME)-nogfx $(OBJECTS) nullrenderer.o nullsound.o sqlite3.o libenet.a $(LIBS)

# implicit rules and compile action for .c files used here
main.o :      $(addprefix include/, linear.h renderer.h brain.h loader.h engine.h clocks.h network.h bsod.h misc.h)
brain.o :     $(addprefix include/, engine.h linear.h sound.h physics.h chunk.h megaman.h doodad.h network.h messages.h clocks.h misc.h)
physics.o :   $(addprefix include/, linear.h doodad.h physics.h chunk.h misc.h)
doodad.o :    $(addprefix include/, linear.h doodad.h)
chunk.o :     $(addprefix include/, linear.h chunk.h floodfill.h bsod.h)
linear.o :    include/linear.h
clocks.o :    include/clocks.h
floodfill.o : include/floodfill.h
messages.o :  include/messages.h
misc.o :      include/misc.h
renderer.o :  $(addprefix include/, raylib.h linear.h engine.h doodad.h megaman.h chunk.h renderer.h bsod.h physics.h sound.h brain.h)
sound.o :     $(addprefix include/, raylib.h sound.h)
loader.o :    $(addprefix include/, loader.h sqlite3.h linear.h doodad.h megaman.h chunk.h)
network.o :   $(addprefix include/, enet/enet.h network.h)
nullsound.o : $(addprefix include/, sound.h)
nullrenderer.o :
sync.o :      $(addprefix include/, sync.h clocks.h messages.h network.h)

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

include/raylib.h include/rlgl.h &: vendor/raylib
	cp vendor/raylib/src/raylib.h include/
	cp vendor/raylib/src/rlgl.h include/

SQLITE_NAME = sqlite-amalgamation-3430000
SQLITE_PATH = vendor/sqlite3/$(SQLITE_NAME)
SQLITE_URL = https://www.sqlite.org/2023/$(SQLITE_NAME).zip

vendor/sqlite3 :
	rm -rf vendor/sqlite3
	mkdir -p vendor/sqlite3
	wget -P vendor/sqlite3/ $(SQLITE_URL)
	unzip vendor/sqlite3/$(SQLITE_NAME).zip -d vendor/sqlite3/

include/sqlite3.h sqlite3.c &: vendor/sqlite3
	cp vendor/sqlite3/$(SQLITE_NAME)/sqlite3.c .
	cp vendor/sqlite3/$(SQLITE_NAME)/sqlite3.h include/

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

tools/spawner : spawner.c
	gcc -Wall -Wextra -o tools/spawner spawner.c

clean :
	rm -f $(EXE_NAME) $(EXE_NAME)-nogfx
	rm -f $(OBJECTS)
	rm -f renderer.o nullrenderer.o sound.o nullsound.o
	rm -f libraylib.a libenet.a
	rm -rf include/enet
	rm -rf include/raylib.h
	rm -rf include/rlgl.h

distclean : clean
	rm -f sqlite3.c
	rm -f sqlite3.o
	rm -rf include/sqlite3.h
	rm -rf vendor/sqlite3/
	rm -rf vendor/raylib/
	rm -rf vendor/enet/
