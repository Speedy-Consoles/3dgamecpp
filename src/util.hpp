#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include "vmath.hpp"

//using namespace std;

extern const double TAU;

extern const vec3i8 DIRS[6];
extern const int DIR_DIMS[6];
extern const int OTHER_DIR_DIMS[6][2];

extern const vec3i QUAD_CYCLES_3D[6][4];
extern const vec2i QUAD_CYCLE_2D[4];
extern const vec3i EIGHT_CYCLES_3D[6][8];
extern const uint8 FACE_CORNER_MASK[4][3];
extern const vec3i8 NINE_CUBE_CYCLE[27];
extern const vec3i8 CUBE_CYCLE[8];
extern const size_t DIR_2_CUBE_CYCLE[6];
extern const size_t BASE_NINE_CUBE_CYCLE;

const uint8 TEST_CORNERS[6] { 0x07, 0xC1, 0x00, 0xC1, 0x07, 0x00 };

extern std::vector<vec3i8> LOADING_ORDER;

template <typename T> int sgn(T val) {return (T(0) < val) - (val < T(0));}

int getDir(int dim, int sign);

size_t vec2CubeCycle(vec3i8 v);

vec3d getVectorFromAngles(double yaw, double pitch);

vec3i64 wc2bc(vec3i64 wc);
vec3i64 bc2cc(vec3i64 bc);
vec3ui8 bc2icc(vec3i64 bc);

size_t vec3i64HashFunc(vec3i64 v);

template<typename T> T clamp(T v, T min, T max) {
	if (v != v)
		return v;
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

void initUtil();

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

#ifdef __GNUC__
#define PACKED(func) func __attribute__((__packed__))
#elif defined(_MSC_VER)
#define PACKED(func) __pragma(pack(push, 1)) func __pragma(pack(pop))
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

#endif // UTIL_HPP
