CLIENT_EXECUTABLE_NAME = 3dgame
SERVER_EXECUTABLE_NAME = 3dgame_srv

CLIENT_OBJECT_FILES = \
	client/client.cpp.o\
	client/config.cpp.o\
	client/graphics.cpp.o\
	client/local_server_interface.cpp.o\
	client/menu.cpp.o\
	client/render.cpp.o\
	client/render_menu.cpp.o\
	client/texture_manager.cpp.o\
	game/chunk.cpp.o\
	game/player.cpp.o\
	game/world.cpp.o\
	gui/button.cpp.o\
	gui/frame.cpp.o\
	gui/label.cpp.o\
	gui/widget.cpp.o\
	io/archive.cpp.o\
	io/chunk_loader.cpp.o\
	io/chunk_loader_internals.cpp.o\
	io/logging.cpp.o\
	monitor.cpp.o\
	perlin.cpp.o\
	stopwatch.cpp.o\
	time.cpp.o\
	util.cpp.o\
	vmath.cpp.o\
	world_generator.cpp.o

SERVER_OBJECT_FILES = \
	server/server.cpp.o\
	game/chunk.cpp.o\
	game/player.cpp.o\
	game/world.cpp.o\
	io/archive.cpp.o\
	io/chunk_loader.cpp.o\
	io/chunk_loader_internals.cpp.o\
	io/logging.cpp.o\
	monitor.cpp.o\
	perlin.cpp.o\
	stopwatch.cpp.o\
	time.cpp.o\
	util.cpp.o\
	vmath.cpp.o\
	world_generator.cpp.o

CXX = g++
LD = g++

CXXFLAGS = -Wall -std=c++11 `freetype-config --cflags` -pthread -Isrc
LDFLAGS = -pthread

CLIENT_LDFLAGS = $(LDFLAGS)
SERVER_LDFLAGS = $(LDFLAGS)
#CXXFLAGS += -DNO_GRAPHICS

CLIENT_LIBS_LD_FLAGS = -llog4cxx -lGL -lGLU -lGLEW -lftgl -lSDL2 -lSDL2_image
SERVER_LIBS_LD_FLAGS = -llog4cxx -lSDL2 -lSDL2_net -lboost_system

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

CLIENT_OBJECTS = $(addprefix $(OBJ_DIR)/,$(CLIENT_OBJECT_FILES))
SERVER_OBJECTS = $(addprefix $(OBJ_DIR)/,$(SERVER_OBJECT_FILES))

CLIENT_EXECUTABLE = $(BIN_DIR)/$(CLIENT_EXECUTABLE_NAME)
SERVER_EXECUTABLE = $(BIN_DIR)/$(SERVER_EXECUTABLE_NAME)

all: client server

client: $(CLIENT_EXECUTABLE)

server: $(SERVER_EXECUTABLE)

dir_guard=@mkdir -p $(@D)

$(OBJ_DIR)/%.cpp.o : src/%.cpp
	$(dir_guard)
	$(CXX) -MMD -c -o $@ $< $(CXXFLAGS)
	@cp $(OBJ_DIR)/$*.cpp.d $(OBJ_DIR)/$*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' \
			< $(OBJ_DIR)/$*.cpp.d >> $(OBJ_DIR)/$*.P; \
			rm -f $(OBJ_DIR)/$*.cpp.d

-include $(OBJECTS:%.cpp.o=%.P)

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECTS)
	$(dir_guard)
	$(LD) $(CLIENT_LDFLAGS) $^ $(CLIENT_LIBS_LD_FLAGS) -o $@

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS)
	$(dir_guard)
	$(LD) $(SERVER_LDFLAGS) $^ $(SERVER_LIBS_LD_FLAGS) -o $@

.PHONY: make_dirs clean all

clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)