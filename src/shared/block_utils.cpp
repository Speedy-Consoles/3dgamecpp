#include "block_utils.hpp"

#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "engine/math.hpp"
#include "engine/logging.hpp"
#include "game/chunk.hpp"

#include "constants.hpp"

static logging::Logger logger("util");

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
std::vector<int> LO_MAX_RADIUS_INDICES;
std::vector<int> LO_INDEX_FINISHED_RADIUS;

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
	vec3i64 result;
	for (int dim = 0; dim < 3; dim++) {
		result[dim] = wc[dim] >> RESOLUTION_EXPONENT;
	}
	return result;
}

vec3i64 bc2cc(vec3i64 bc) {
	vec3i64 result;
	for (int dim = 0; dim < 3; dim++) {
			result[dim] = bc[dim] >> Chunk::WIDTH_EXPONENT;
	}
	return result;
}

vec3ui8 bc2icc(vec3i64 bc) {
	return vec3ui8(
		(bc[0] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH,
		(bc[1] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH,
		(bc[2] % Chunk::WIDTH + Chunk::WIDTH) % Chunk::WIDTH
	);
}

// TODO why can't gridSize and return type be size_t?
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

bool vec3i64CompFunc(vec3i64 v1, vec3i64 v2) {
	int n1 = v1.norm2();
	int n2 = v2.norm2();
	if (n1 > n2)
		return false;
	if (n1 < n2)
		return true;
	if (v1[2] > v2[2])
		return false;
	if (v1[2] < v2[2])
		return true;
	if (v1[1] > v2[1])
		return false;
	if (v1[1] < v2[1])
		return true;
	return v1[0] < v2[0];
}

void initUtil() {
	LOG_INFO(logger) << "Initializing loading order";
	int range = MAX_RENDER_DISTANCE;
	int length = range * 2 + 1;
	std::vector<vec3i8> strictOrder;
	strictOrder.resize(length * length * length);
	std::unordered_set<vec3i64, size_t(*)(vec3i64)> inserted(0, vec3i64HashFunc);

	for (int i = 0, z = -range; z <= range; z++) {
		for (int y = -range; y <= range; y++) {
			for (int x = -range; x <= range; x++) {
				strictOrder[i++] = vec3i8(x, y, z);
			}
		}
	}

	auto comp = [](vec3i8 v1, vec3i8 v2) {
		return v1.norm2() < v2.norm2();
	};
	std::sort(strictOrder.begin(), strictOrder.end(), comp);

	LOADING_ORDER.reserve(length * length * length);
	int bubbleRadius = 6;
	for (int i = 0; i < length * length * length; i++) {
		if (inserted.find(strictOrder[i].cast<int64>()) != inserted.end())
			continue;
		for (int j = 0; j < length * length * length; j++) {
			if (strictOrder[j].norm() > bubbleRadius)
				break;
			vec3i8 newVec = strictOrder[i] + strictOrder[j];
			if (newVec.maxAbs() > range)
				continue;
			if (inserted.find(newVec.cast<int64>()) != inserted.end())
				continue;
			LOADING_ORDER.push_back(newVec);
			inserted.insert(newVec.cast<int64>());
		}
	}

	int biggestRadius = std::ceil(std::sqrt(3) * length / 2.0);
	LO_MAX_RADIUS_INDICES.resize(biggestRadius + 1, -1);
	LO_INDEX_FINISHED_RADIUS.resize(length * length * length, -1);

	int maxRadius = 0;
	for (int i = 0; i < length * length * length; ++i) {
		double dist = LOADING_ORDER[i].norm();
		while (dist > maxRadius) {
			LO_MAX_RADIUS_INDICES[maxRadius] = i;
			maxRadius++;
		}
	}
	int finishedRadius = biggestRadius;
	for (int i = length * length * length - 1; i >= 0; --i) {
		double dist = LOADING_ORDER[i].norm();
		if (dist <= finishedRadius) {
			finishedRadius = std::ceil(dist) - 1;
		}
		LO_INDEX_FINISHED_RADIUS[i] = finishedRadius;
	}
}
