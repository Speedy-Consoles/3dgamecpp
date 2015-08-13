#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "shared/engine/vmath.hpp"

struct GraphicsConf;

class Renderer {
public:
	virtual ~Renderer() = default;

	virtual void tick() = 0;
	virtual void render() = 0;
	virtual void resize() = 0;
	virtual void setConf(const GraphicsConf &, const GraphicsConf &) = 0;

	virtual void rebuildChunk(vec3i64 chunkCoords) = 0;

	virtual float getMaxFOV() = 0;
};

#endif // RENDERER_HPP
