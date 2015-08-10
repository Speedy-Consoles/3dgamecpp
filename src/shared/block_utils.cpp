#include "block_utils.hpp"

#include <algorithm>

#include "engine/math.hpp"
#include "game/chunk.hpp"

#include "constants.hpp"

const vec3i8 DIRS[6] = {
	{ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
	{-1, 0, 0 }, { 0,-1, 0 }, { 0, 0,-1 }
};

const int DIR_DIMS[6] = { 0, 1, 2, 0, 1, 2 };

const int OTHER_DIR_DIMS[6][2] = {
	{ 1, 2 }, { 2, 0 }, { 0, 1 },
	{ 1, 2 }, { 2, 0 }, { 0, 1 }
};

const vec3i DIR_QUAD_CORNER_CYCLES_3D[6][4] = {
	{{ 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 1, 0, 1 }}, // right
	{{ 1, 1, 0 }, { 0, 1, 0 }, { 0, 1, 1 }, { 1, 1, 1 }}, // back
	{{ 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 }}, // top
	{{ 0, 1, 0 }, { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 1 }}, // left
	{{ 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 1 }, { 0, 0, 1 }}, // front
	{{ 1, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }}, // bottom
};

const vec2i QUAD_CORNER_CYCLE[4] = {{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }};

const vec3i DIR_QUAD_EIGHT_NEIGHBOR_CYCLES[6][8] = {
	{{ 1,-1,-1 }, { 1, 0,-1 }, { 1, 1,-1 }, { 1, 1, 0 }, { 1, 1, 1 }, { 1, 0, 1 }, { 1,-1, 1 }, { 1,-1, 0 }},
	{{ 1, 1,-1 }, { 0, 1,-1 }, {-1, 1,-1 }, {-1, 1, 0 }, {-1, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 1, 0 }},
	{{-1,-1, 1 }, { 0,-1, 1 }, { 1,-1, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 }, {-1, 1, 1 }, {-1, 0, 1 }},
	{{-1, 1,-1 }, {-1, 0,-1 }, {-1,-1,-1 }, {-1,-1, 0 }, {-1,-1, 1 }, {-1, 0, 1 }, {-1, 1, 1 }, {-1, 1, 0 }},
	{{-1,-1,-1 }, { 0,-1,-1 }, { 1,-1,-1 }, { 1,-1, 0 }, { 1,-1, 1 }, { 0,-1, 1 }, {-1,-1, 1 }, {-1,-1, 0 }},
	{{ 1,-1,-1 }, { 0,-1,-1 }, {-1,-1,-1 }, {-1, 0,-1 }, {-1, 1,-1 }, { 0, 1,-1 }, { 1, 1,-1 }, { 1, 0,-1 }}
};

const uint8 QUAD_CORNER_MASK[4][3] = {
	{0x80, 0x01, 0x02},
	{0x02, 0x04, 0x08},
	{0x08, 0x10, 0x20},
	{0x20, 0x40, 0x80},
};

const vec3i8 BIG_CUBE_CYCLE[27] = {
	{-1,-1,-1 }, { 0,-1,-1 }, { 1,-1,-1 },
	{-1, 0,-1 }, { 0, 0,-1 }, { 1, 0,-1 },
	{-1, 1,-1 }, { 0, 1,-1 }, { 1, 1,-1 },
	{-1,-1, 0 }, { 0,-1, 0 }, { 1,-1, 0 },
	{-1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 },
	{-1, 1, 0 }, { 0, 1, 0 }, { 1, 1, 0 },
	{-1,-1, 1 }, { 0,-1, 1 }, { 1,-1, 1 },
	{-1, 0, 1 }, { 0, 0, 1 }, { 1, 0, 1 },
	{-1, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 }
};

const vec3i8 CUBE_CORNER_CYCLE[8] = {
	{ 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
	{ 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 1 }, { 0, 0, 1 }
};

const size_t DIR_TO_BIG_CUBE_CYCLE_INDEX[6] = { 14, 16, 22, 12, 10, 4 };

const size_t BIG_CUBE_CYCLE_BASE_INDEX = 13;

std::vector<vec3i8> LOADING_ORDER;
std::vector<int> LOADING_ORDER_DISTANCE_INDICES;

int getDir(int dim, int sign) {
	return dim - 3 * ((sign - 1) / 2);
}

size_t vec2BigCubeCycleIndex(vec3i8 v) {
	return (v[2] + 1) * 9 + (v[1] + 1) * 3 + v[0] + 1;
}

vec3d getVectorFromAngles(double yaw, double pitch) {
	double xyLength = cos(pitch * TAU / 360);
	return vec3d(
		cos(yaw * TAU / 360) * xyLength,
		sin(yaw * TAU / 360) * xyLength,
		sin(pitch * TAU / 360)
	);
}

vec3i64 wc2bc(vec3i64 wc) {
	return vec3i64(
		(int64) floor(wc[0] / (float) RESOLUTION),
		(int64) floor(wc[1] / (float) RESOLUTION),
		(int64) floor(wc[2] / (float) RESOLUTION)
	);
}

vec3i64 bc2cc(vec3i64 bc) {
	return vec3i64(
		(int64) floor(bc[0] / (float) Chunk::WIDTH),
		(int64) floor(bc[1] / (float) Chunk::WIDTH),
		(int64) floor(bc[2] / (float) Chunk::WIDTH)
	);
}

vec3ui8 bc2icc(vec3i64 bc) {
	return vec3ui8(
		(bc[0] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH,
		(bc[1] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH,
		(bc[2] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH
	);
}

int64 gridCycleIndex(vec3i64 v, int64 gridSize) {
	return (cycle(v[2], gridSize) * gridSize
			+ cycle(v[1], gridSize)) * gridSize
			+ cycle(v[0], gridSize);
}

size_t vec3i64HashFunc(vec3i64 v) {
	static const int prime = 31;
	size_t result = 1;
	result = prime * result + (size_t) (v[0] ^ (v[0] >> 32));
	result = prime * result + (size_t) (v[1] ^ (v[1] >> 32));
	result = prime * result + (size_t) (v[2] ^ (v[2] >> 32));
	return result;
}

void initUtil() {
	int range = 32;
	int length = range * 2 + 1;
	LOADING_ORDER.resize(length * length * length);

	int i = 0;
	for (int z = -range; z <= range; z++) {
		for (int y = -range; y <= range; y++) {
			for (int x = -range; x <= range; x++) {
				LOADING_ORDER[i++] = vec3i8(x, y, z);
			}
		}
	}

	auto comp = [](vec3i8 v1, vec3i8 v2){return v1.norm2() < v2.norm2();};
	std::sort(LOADING_ORDER.begin(), LOADING_ORDER.end(), comp);

	LOADING_ORDER_DISTANCE_INDICES.resize(range + 1);
	int distance = 0;
	for (int i = 0; i < length * length * length && distance <= range; ++i) {
	    vec3i8 coords = LOADING_ORDER[i];
	    if (coords.norm() >= distance) {
	    	LOADING_ORDER_DISTANCE_INDICES[distance++] = i;
	    }
	}
}
