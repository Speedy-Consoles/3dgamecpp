#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include <chrono>
#include "vmath.hpp"

using namespace std::chrono;

extern const double TAU;

extern const vec3i8 DIRS[6];
extern const int DIR_DIMS[6];
extern const int OTHER_DIR_DIMS[6][2];

extern const vec3i QUAD_CYCLES_3D[6][4];
extern const vec2i QUAD_CYCLE_2D[4];

extern std::vector<vec3i8> LOADING_ORDER;

template <typename T> int sgn(T val) {return (T(0) < val) - (val < T(0));}

int getDir(int dim, int sign);

vec3d getVectorFromAngles(double yaw, double pitch);

vec3i64 wc2bc(vec3i64 wc);
vec3i64 bc2cc(vec3i64 bc);
vec3ui8 bc2icc(vec3i64 bc);

size_t vec3i64HashFunc(vec3i64 v);

int64 getMicroTimeSince(time_point<high_resolution_clock>);

void initUtil();

#endif // UTIL_HPP
