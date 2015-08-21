# client stuff
CLIENT_EXECUTABLE_NAME = 3dgame
CLIENT_OBJECT_FILES = \
	client/client.cpp.o\
	client/config.cpp.o\
	client/local_server_interface.cpp.o\
	client/menu.cpp.o\
	client/remote_server_interface.cpp.o\
	client/gfx/font.cpp.o\
	client/gfx/graphics.cpp.o\
	client/gfx/chunk_renderer.cpp.o\
	client/gfx/texture_loader.cpp.o\
	client/gfx/texture_manager.cpp.o\
	client/gfx/gl2/gl2_renderer.cpp.o\
	client/gfx/gl2/gl2_chunk_renderer.cpp.o\
	client/gfx/gl2/gl2_target_renderer.cpp.o\
	client/gfx/gl2/gl2_crosshair_renderer.cpp.o\
	client/gfx/gl2/gl2_sky_renderer.cpp.o\
	client/gfx/gl2/gl2_hud_renderer.cpp.o\
	client/gfx/gl2/gl2_debug_renderer.cpp.o\
	client/gfx/gl2/gl2_menu_renderer.cpp.o\
	client/gfx/gl2/gl2_texture_manager.cpp.o\
	client/gfx/gl3/gl3_renderer.cpp.o\
	client/gfx/gl3/gl3_chunk_renderer.cpp.o\
	client/gfx/gl3/gl3_sky_renderer.cpp.o\
	client/gfx/gl3/gl3_target_renderer.cpp.o\
	client/gfx/gl3/gl3_crosshair_renderer.cpp.o\
	client/gfx/gl3/gl3_hud_renderer.cpp.o\
	client/gfx/gl3/gl3_debug_renderer.cpp.o\
	client/gfx/gl3/gl3_menu_renderer.cpp.o\
	client/gfx/gl3/gl3_texture_manager.cpp.o\
	client/gfx/gl3/gl3_shaders.cpp.o\
	client/gfx/gl3/gl3_font.cpp.o\
	client/gui/button.cpp.o\
	client/gui/label.cpp.o\
	client/gui/widget.cpp.o

# server stuff
SERVER_EXECUTABLE_NAME = 3dgame_srv
SERVER_OBJECT_FILES = \
	server/server.cpp.o

# test stuff
TEST_EXECUTABLE_NAME = test
TEST_OBJECT_FILES = \
	test/test_chunk_archive.cpp.o\
	test/test_loading_order.cpp.o

# stuff needed by both client and server
SHARED_ARCHIVE_NAME = shared_archive
SHARED_OBJECT_FILES = \
	shared/engine/buffer.cpp.o\
	shared/engine/logging.cpp.o\
	shared/engine/socket.cpp.o\
	shared/engine/stopwatch.cpp.o\
	shared/engine/thread.cpp.o\
	shared/engine/time.cpp.o\
	shared/engine/unicode_int.cpp.o\
	shared/game/chunk.cpp.o\
	shared/game/perlin.cpp.o\
	shared/game/player.cpp.o\
	shared/game/world.cpp.o\
	shared/game/world_generator.cpp.o\
	shared/block_loader.cpp.o\
	shared/block_manager.cpp.o\
	shared/block_utils.cpp.o\
	shared/chunk_archive.cpp.o\
	shared/chunk_manager.cpp.o\
	shared/net.cpp.o

# what programs to use
CXX = g++
LD = g++
AR = ar crs

# general flags
CXXFLAGS = -Wall -Wextra -std=c++11 `freetype-config --cflags` -pthread -Isrc
LDFLAGS = -pthread
LIBS_LD_FLAGS = -llog4cxx -lboost_system -lboost_filesystem

#CXXFLAGS += -DNO_GRAPHICS

# program specific flags
CLIENT_LDFLAGS = $(LDFLAGS)
SERVER_LDFLAGS = $(LDFLAGS)
TEST_LDFLAGS = $(LDFLAGS)
CLIENT_LIBS_LD_FLAGS = $(LIBS_LD_FLAGS)
SERVER_LIBS_LD_FLAGS = $(LIBS_LD_FLAGS)
TEST_LIBS_LD_FLAGS = $(LIBS_LD_FLAGS)

TEST_LIBS_LD_FLAGS += -lgtest -lgtest_main
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
TEST_OBJECTS = $(addprefix $(OBJ_DIR)/,$(TEST_OBJECT_FILES))
SHARED_OBJECTS = $(addprefix $(OBJ_DIR)/,$(SHARED_OBJECT_FILES))
OBJECTS = $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(SHARED_OBJECTS) $(TEST_OBJECTS)

CLIENT_EXECUTABLE = $(BIN_DIR)/$(CLIENT_EXECUTABLE_NAME)
SERVER_EXECUTABLE = $(BIN_DIR)/$(SERVER_EXECUTABLE_NAME)
TEST_EXECUTABLE = $(BIN_DIR)/$(TEST_EXECUTABLE_NAME)
SHARED_ARCHIVE = $(OBJ_DIR)/$(SHARED_ARCHIVE_NAME).a

# targets
all: client server test

client: $(CLIENT_EXECUTABLE)
server: $(SERVER_EXECUTABLE)
test: $(TEST_EXECUTABLE)

$(SHARED_ARCHIVE): $(SHARED_OBJECTS)

clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)

.PHONY: clean all client server test

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

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(SHARED_ARCHIVE)
	$(dir_guard)
	$(LD) $(TEST_LDFLAGS) -o $@ $^ $(TEST_LIBS_LD_FLAGS)

