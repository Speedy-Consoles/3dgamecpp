#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "client/graphics.hpp"
#include "client/bmfont.hpp"
#include "game/world.hpp"

class GL3Renderer;

class GL3DebugRenderer {
public:
	GL3DebugRenderer(Client *client, Graphics *graphics, GL3Renderer *renderer, Shaders *shaders, const World *world, const uint8 *playerId);

	void render();

private:
	Client *client = nullptr;
	Graphics *graphics = nullptr;
	GL3Renderer *renderer = nullptr;
	const World *world = nullptr;
	const uint8 &playerId;

	BMFont font;
};

#endif //GL3_DEBUG_RENDERER_HPP_
