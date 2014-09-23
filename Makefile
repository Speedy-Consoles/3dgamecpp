EXECUTABLE_NAME = 3dgame

SOURCE_FILES = \
	gui/button.cpp\
	gui/frame.cpp\
	gui/label.cpp\
	gui/widget.cpp\
	io/archive.cpp\
	io/chunk_loader.cpp\
	io/chunk_loader_internals.cpp\
	io/logging.cpp\
	client/client.cpp\
	client/config.cpp\
	client/graphics.cpp\
	client/local_server_interface.cpp\
	client/menu.cpp\
	client/render.cpp\
	client/render_menu.cpp\
	client/texture_manager.cpp\
	game/chunk.cpp\
	game/player.cpp\
	game/world.cpp\
	monitor.cpp\
	perlin.cpp\
	stopwatch.cpp\
	util.cpp\
	vmath.cpp\
	world_generator.cpp

CXX = g++
LD = g++

CXXFLAGS = -Wall -std=c++11 `freetype-config --cflags` -pthread -Isrc
LDFLAGS = -pthread

#CXXFLAGS += -DNO_GRAPHICS

LIB_DIRS = 

LIBS_LD_FLAGS  = -Wl,-Bstatic 
LIBS_LD_FLAGS += -Wl,-Bdynamic -llog4cxx -lGL -lGLU -lGLEW -lftgl -lSDL2 -lSDL2_image

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

EXECUTABLE = $(BIN_DIR)/$(EXECUTABLE_NAME)

OBJECTS = $(SOURCE_FILES:%.cpp=$(OBJ_DIR)/%.cpp.o)

all: $(EXECUTABLE)

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

$(EXECUTABLE): $(OBJECTS)
	$(dir_guard)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS_LD_FLAGS) -o $@

.PHONY: make_dirs clean all

clean:
	rm -Rf $(OBJ_DIR)
	rm -Rf $(BIN_DIR)