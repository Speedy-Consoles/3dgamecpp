#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "client/graphics.hpp"
#include "client/bmfont.hpp"
#include "game/world.hpp"

class GL3Renderer;

class GL3DebugRenderer {
public:
	GL3DebugRenderer(Graphics *graphics, GL3Renderer *renderer, Shaders *shaders, const World *world, int playerId);

	void render();

private:
	Graphics *graphics = nullptr;
	GL3Renderer *renderer = nullptr;
	const World *world = nullptr;
	int playerId = 0;

	BMFont font;
};

#endif //GL3_DEBUG_RENDERER_HPP_
