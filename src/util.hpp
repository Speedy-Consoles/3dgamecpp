#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>

#include "engine/vmath.hpp"

extern const vec3i8 DIRS[6];
extern const int DIR_DIMS[6];
extern const int OTHER_DIR_DIMS[6][2];

extern const vec3i DIR_QUAD_CORNER_CYCLES_3D[6][4];
extern const vec2i QUAD_CORNER_CYCLE[4];
extern const vec3i DIR_QUAD_EIGHT_NEIGHBOR_CYCLES[6][8];
extern const vec3i8 CUBE_CORNER_CYCLE[8];
extern const vec3i8 BIG_CUBE_CYCLE[27];
extern const size_t DIR_TO_BIG_CUBE_CYCLE_INDEX[6];
extern const size_t BIG_CUBE_CYCLE_BASE_INDEX;

extern const uint8 QUAD_CORNER_MASK[4][3];

const uint8 TEST_CORNERS[6] { 0x07, 0xC1, 0x00, 0xC1, 0x07, 0x00 };

extern std::vector<vec3i8> LOADING_ORDER;
extern std::vector<int> LOADING_ORDER_DISTANCE_INDICES;

template <typename T> int sgn(T val) {return (T(0) < val) - (val < T(0));}

int getDir(int dim, int sign);

size_t vec2BigCubeCycleIndex(vec3i8 v);

vec3d getVectorFromAngles(double yaw, double pitch);

vec3i64 wc2bc(vec3i64 wc);
vec3i64 bc2cc(vec3i64 bc);
vec3ui8 bc2icc(vec3i64 bc);

int64 gridCycleIndex(vec3i64 v, int64 gridSize);

size_t vec3i64HashFunc(vec3i64 v);

void initUtil();

enum FaceDir {
	DIR_EAST,
	DIR_NORTH,
	DIR_UP,
	DIR_WEST,
	DIR_SOUTH,
	DIR_DOWN,
};

enum FaceDirMask {
	MASK_EAST  = 1 << DIR_EAST,
	MASK_NORTH = 1 << DIR_NORTH,
	MASK_UP    = 1 << DIR_UP,
	MASK_WEST  = 1 << DIR_WEST,
	MASK_SOUTH = 1 << DIR_SOUTH,
	MASK_DOWN  = 1 << DIR_DOWN,
	
	MASK_NONE  = 0,
	MASK_SIDES = MASK_NORTH | MASK_EAST | MASK_SOUTH | MASK_WEST,
	MASK_VERT  = MASK_UP | MASK_DOWN,
	MASK_ALL   = MASK_SIDES | MASK_VERT,
};

#endif // UTIL_HPP
