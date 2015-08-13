#include "gl3_hud_renderer.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "client/client.hpp"
#include "shared/engine/logging.hpp"

#include "gl3_renderer.hpp"

GL3HudRenderer::GL3HudRenderer(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	// nothing
}

void GL3HudRenderer::render() {
	if (client->getState() != Client::State::PLAYING)
		return;

	const Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	return;
}
