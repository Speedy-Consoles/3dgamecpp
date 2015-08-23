#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "engine/std_types.hpp"

static const uint RESOLUTION_EXPONENT = 10;
static const uint RESOLUTION = 1 << RESOLUTION_EXPONENT;
static const uint MAX_RENDER_DISTANCE = 64;
static const uint MAX_RENDER_CUBE_DIAMETER = MAX_RENDER_DISTANCE * 2 + 1;
static const uint MAX_RENDER_CUBE_SIZE =
		MAX_RENDER_CUBE_DIAMETER
		* MAX_RENDER_CUBE_DIAMETER
		* MAX_RENDER_CUBE_DIAMETER;
static const uint8 MAX_CLIENTS = 16;
static const uint TICK_SPEED = 60;

#endif // CONSTANTS_HPP
