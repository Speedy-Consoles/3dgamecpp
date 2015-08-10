#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "shared/game/world.hpp"
#include "client/gfx/graphics.hpp"

#include "gl3_font.hpp"

class GL3Renderer;
class GL3ChunkRenderer;

class GL3DebugRenderer {
public:
	GL3DebugRenderer(Client *client, GL3Renderer *renderer, GL3ChunkRenderer *chunkRenderer, ShaderManager *shaderManager);

	void render();

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;
	GL3ChunkRenderer *chunkRenderer = nullptr;

	BMFont font;
};

#endif //GL3_DEBUG_RENDERER_HPP_
