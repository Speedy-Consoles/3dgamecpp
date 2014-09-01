CPP = g++
LD = g++

CPPFLAGS = -Wall -std=c++11 -O2
LDFLAGS =
LIBS = -lSDL2 -lGL -lGLU -lGLEW -lSDL2_image -lftgl
INCLUDE_DIRS = -I/usr/include/freetype2

EXECUTABLE = bin/3dgame

SOURCE_FILES = \
	chunk_loader.cpp\
	chunk.cpp client.cpp\
	graphics.cpp\
	local_server_interface.cpp\
	perlin.cpp\
	player.cpp\
	util.cpp\
	world.cpp

OBJECTS = $(SOURCE_FILES:%.cpp=obj/%.cpp.o)

all: make_dirs $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

obj/%.cpp.o: src/%.cpp
	$(CPP) $(INCLUDE_DIRS) -c -o $@ $^ $(CPPFLAGS)

.PHONY: make_dirs clean

make_dirs:
	mkdir -p obj
	mkdir -p bin

clean:
	rm obj/*
	rm bin/*