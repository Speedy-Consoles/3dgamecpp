#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "shared/engine/vmath.hpp"

struct GraphicsConf;

class Renderer {
public:
	virtual ~Renderer() = default;

	virtual void tick() = 0;
	virtual void resize() = 0;
	virtual void setConf(const GraphicsConf &, const GraphicsConf &) = 0;

	virtual void rerenderChunk(vec3i64 chunkCoords) = 0;
};

#endif // RENDERER_HPP
