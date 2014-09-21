EXECUTABLE_NAME = 3dgame

SOURCE_FILES = \
	archive.cpp\
	chunk_loader.cpp\
	chunk_loader_internals.cpp\
	chunk.cpp\
	client.cpp\
	config.cpp\
	graphics.cpp\
	local_server_interface.cpp\
	logging.cpp\
	menu.cpp\
	monitor.cpp\
	perlin.cpp\
	player.cpp\
	render.cpp\
	render_menu.cpp\
	stopwatch.cpp\
	texture_manager.cpp\
	util.cpp\
	vmath.cpp\
	world.cpp\
	world_generator.cpp

CXX = g++
LD = g++

CXXFLAGS = -Wall -std=c++11 `freetype-config --cflags` -pthread
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