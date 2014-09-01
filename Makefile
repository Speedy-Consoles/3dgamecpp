EXECUTABLE_NAME = 3dgame

SOURCE_FILES = \
	chunk_loader.cpp\
	chunk.cpp client.cpp\
	graphics.cpp\
	local_server_interface.cpp\
	perlin.cpp\
	player.cpp\
	util.cpp\
	world.cpp

CXX = g++
LD = g++

CXXFLAGS = -Wall -std=c++11
LDFLAGS =

INCLUDE_DIRS = /usr/include/freetype2
LIB_DIRS = 

LIBS = SDL2 GL GLU GLEW SDL2_image ftgl

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g
	OBJ_DIR = obj/debug
	BIN_DIR = bin/debug
else
	CXXFLAGS += -O2 -s
	OBJ_DIR = obj/release
	BIN_DIR = bin/release
endif

CXXFLAGS += $(addprefix -I, $(INCLUDE_DIRS))
LDFLAGS += $(addprefix -L, $(LIB_DIRS))

LIBS_LD_FLAGS = $(addprefix -l, $(LIBS))

EXECUTABLE = $(BIN_DIR)/$(EXECUTABLE_NAME)

OBJECTS = $(SOURCE_FILES:%.cpp=$(OBJ_DIR)/%.cpp.o)

all: make_dirs $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS_LD_FLAGS) -o $@

$(OBJ_DIR)/%.cpp.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

.PHONY: make_dirs clean all

make_dirs:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

clean:
	rm $(OBJ_DIR)/*
	rm $(BIN_DIR)/*