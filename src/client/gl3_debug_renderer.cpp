#include "gl3_debug_renderer.hpp"

GL3DebugRenderer::GL3DebugRenderer(Client *client, GL3Renderer *renderer, Shaders *shaders, Graphics *graphics) :
	client(client),
	renderer(renderer),
	graphics(graphics),
	font(shaders)
{
	font.load("fonts/dejavusansmono24.fnt");
	font.setEncoding(Font::Encoding::UTF8);
}

void GL3DebugRenderer::render() {
	const Player &player = client->getWorld()->getPlayer(client->getLocalClientId());

	float x = -graphics->getDrawWidth() / 2 + 5;
	float y = graphics->getDrawHeight() / 2 - font.getTopOffset() - 5;

	char buffer[1024];
#define RENDER_LINE(...) sprintf(buffer, __VA_ARGS__);\
			font.write(x, y, 0.0f, buffer, 0);\
			y -= font.getLineHeight()

	RENDER_LINE("x: %" PRId64 "(%" PRId64 ")", player.getPos()[0],
		(int64)floor(player.getPos()[0] / (double)RESOLUTION));
	RENDER_LINE("y: %" PRId64 " (%" PRId64 ")", player.getPos()[1],
		(int64)floor(player.getPos()[1] / (double)RESOLUTION));
	RENDER_LINE("z: %" PRId64 " (%" PRId64 ")", player.getPos()[2],
		(int64)floor((player.getPos()[2] - Player::EYE_HEIGHT - 1) / (double)RESOLUTION));
	RENDER_LINE("yaw:   %6.1f", player.getYaw());
	RENDER_LINE("pitch: %6.1f", player.getPitch());
	RENDER_LINE("xvel: %8.1f", player.getVel()[0]);
	RENDER_LINE("yvel: %8.1f", player.getVel()[1]);
	RENDER_LINE("zvel: %8.1f", player.getVel()[2]);
}
