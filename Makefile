# client stuff
CLIENT_EXECUTABLE_NAME = 3dgame
CLIENT_OBJECT_FILES = \
	client/client.cpp.o\
	client/config.cpp.o\
	client/graphics.cpp.o\
	client/local_server_interface.cpp.o\
	client/remote_server_interface.cpp.o\
	client/menu.cpp.o\
	client/render.cpp.o\
	client/render_menu.cpp.o\
	client/texture_manager.cpp.o\
	gui/button.cpp.o\
	gui/frame.cpp.o\
	gui/label.cpp.o\
	gui/widget.cpp.o

# server stuff
SERVER_EXECUTABLE_NAME = 3dgame_srv
SERVER_OBJECT_FILES = \
	server/server.cpp.o

# stuff needed by both client and server
SHARED_ARCHIVE_NAME = shared_archive
SHARED_OBJECT_FILES = \
	game/chunk.cpp.o\
	game/player.cpp.o\
	game/world.cpp.o\
	io/archive.cpp.o\
	io/chunk_loader.cpp.o\
	io/chunk_loader_internals.cpp.o\
	io/logging.cpp.o\
	net/buffer.cpp.o\
	net/socket.cpp.o\
	net/net.cpp.o\
	monitor.cpp.o\
	perlin.cpp.o\
	stopwatch.cpp.o\
	time.cpp.o\
	util.cpp.o\
	world_generator.cpp.o

# what programs to use
CXX = g++
LD = g++
AR = ar crs

# general flags
CXXFLAGS = -Wall -std=c++11 `freetype-config --cflags` -pthread -Isrc
LDFLAGS = -pthread
LIBS_LD_FLAGS = -llog4cxx -lboost_system

#CXXFLAGS += -DNO_GRAPHICS

# program specific flags
CLIENT_LDFLAGS = $(LDFLAGS)
SERVER_LDFLAGS = $(LDFLAGS)
CLIENT_LIBS_LD_FLAGS = $(LIBS_LD_FLAGS)
SERVER_LIBS_LD_FLAGS = $(LIBS_LD_FLAGS)

CLIENT_LIBS_LD_FLAGS += -lGL -lGLU -lGLEW -lftgl -lSDL2 -lSDL2_image

# target specific flags
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

# assembling some file paths
CLIENT_OBJECTS = $(addprefix $(OBJ_DIR)/,$(CLIENT_OBJECT_FILES))
SERVER_OBJECTS = $(addprefix $(OBJ_DIR)/,$(SERVER_OBJECT_FILES))
SHARED_OBJECTS = $(addprefix $(OBJ_DIR)/,$(SHARED_OBJECT_FILES))
OBJECTS = $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(SHARED_OBJECTS)

CLIENT_EXECUTABLE = $(BIN_DIR)/$(CLIENT_EXECUTABLE_NAME)
SERVER_EXECUTABLE = $(BIN_DIR)/$(SERVER_EXECUTABLE_NAME)
SHARED_ARCHIVE = $(OBJ_DIR)/$(SHARED_ARCHIVE_NAME).a

# targets
all: client server

client: $(CLIENT_EXECUTABLE)
server: $(SERVER_EXECUTABLE)

$(SHARED_ARCHIVE): $(SHARED_OBJECTS)

clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)

.PHONY: clean all

# creates directories a file is on
dir_guard=@mkdir -p $(@D)

# general build rules
$(OBJ_DIR)/%.cpp.o: src/%.cpp
	$(dir_guard)
	$(CXX) -MMD -c -o $@ $< $(CXXFLAGS)
	@cp $(OBJ_DIR)/$*.cpp.d $(OBJ_DIR)/$*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' \
			< $(OBJ_DIR)/$*.cpp.d >> $(OBJ_DIR)/$*.P; \
			rm -f $(OBJ_DIR)/$*.cpp.d

-include $(OBJECTS:%.cpp.o=%.P)

$(OBJ_DIR)/%.a:
	$(dir_guard)
	$(AR) $@ $^

# specific build rules
$(CLIENT_EXECUTABLE): $(CLIENT_OBJECTS) $(SHARED_ARCHIVE)
	$(dir_guard)
	$(LD) $(CLIENT_LDFLAGS) -o $@ $^ $(CLIENT_LIBS_LD_FLAGS)

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS) $(SHARED_ARCHIVE)
	$(dir_guard)
	$(LD) $(SERVER_LDFLAGS) -o $@ $^ $(SERVER_LIBS_LD_FLAGS)

