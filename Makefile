RAYLIB_VERSION = 4.5.0
RAYLIB_PATH = vendor/raylib/src
RAYLIB_URL = https://github.com/raysan5/raylib.git

SQLITE_NAME = sqlite-amalgamation-3430000
SQLITE_PATH = vendor/sqlite3/$(SQLITE_NAME)
SQLITE_URL = https://www.sqlite.org/2023/$(SQLITE_NAME).zip

EXE_NAME = game
CC = gcc
CFLAGS = \
	-g \
	-O2 \
	-I. \
	-I$(RAYLIB_PATH) \
	-I$(SQLITE_PATH) \
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
	doodad.o

ARCHIVES = $(RAYLIB_PATH)/libraylib.a

$(EXE_NAME) : $(ARCHIVES) $(OBJECTS) main.o sqlite3.o
	gcc -o $(EXE_NAME) $(OBJECTS) main.o sqlite3.o $(ARCHIVES) $(LIBS)

main.o : $(SQLITE_PATH)/sqlite3.h $(RAYLIB_PATH)/raylib.h floodfill.h linear.h bresenham.h
linear.o : linear.h
bresenham.o : bresenham.h
floodfill.o : floodfill.h
doodad.o : symbols.h linear.h doodad.h


symbols.o : symbols.c
	$(CC) -c $(CFLAGS) symbols.c

symbols.c : tools/symgen symbols.def
	tools/symgen symbols.def

tools/symgen : symgen.c
	gcc -Wall -Werror -o tools/symgen symgen.c

sqlite3.o : $(SQLITE_PATH)/sqlite3.c
	$(CC) -c -Wall $(SQLITE_PATH)/sqlite3.c

$(RAYLIB_PATH)/libraylib.a $(RAYLIB_PATH)/raylib.h &: vendor/$(RAYLIB_NAME)
	$(MAKE) -C $(RAYLIB_PATH)

vendor/$(RAYLIB_NAME) :
	mkdir -p vendor
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) vendor/raylib

$(SQLITE_PATH)/sqlite3.h $(SQLITE_PATH)/sqlite3.c &:
	rm -rf vendor/sqlite3
	mkdir -p vendor/sqlite3
	wget -P vendor/sqlite3/ $(SQLITE_URL)
	unzip vendor/sqlite3/$(SQLITE_NAME).zip -d vendor/sqlite3/

clean :
	rm -f $(EXE_NAME) main.o $(OBJECTS)
	rm -f tools/symgen
	rm -f symbols.c symbols.h

veryclean :
	rm -f $(EXE_NAME) main.o sqlite3.o $(OBJECTS)
	rm -f tools/symgen
	rm -f symbols.c symbols.h
	rm -rf vendor/sqlite3/
