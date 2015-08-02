#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "client/graphics.hpp"
#include "client/bmfont.hpp"
#include "game/world.hpp"

class GL3Renderer;

class GL3DebugRenderer {
public:
	GL3DebugRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager);

	void render();

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;

	BMFont font;
};

#endif //GL3_DEBUG_RENDERER_HPP_
