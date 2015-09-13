#include "gl2_player_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "client/config.hpp"

#include "gl2_renderer.hpp"

GL2PlayerRenderer::GL2PlayerRenderer(Client *client, GL2Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	// nothing
}

void GL2PlayerRenderer::render() {
	vec3i64 pos = client->getLocalPlayer().getPos();
	GL(BindTexture(GL_TEXTURE_2D, 0));
	for (uint i = 0; i < MAX_CLIENTS; i++) {
		if (i == client->getLocalClientId())
			continue;
		Player &player = client->getWorld()->getPlayer(i);
		if (!player.isValid())
			continue;
		vec3f pDiff = (player.getPos() - pos).cast<float>() * (1.0 / RESOLUTION);

		GL(PushMatrix());
		GL(Translatef(pDiff[0], pDiff[1], pDiff[2]));
		GL(Rotatef(player.getYaw() / 100.0f, 0, 0, 1));
		GL(Rotated(-player.getPitch() / 100.0f, 0, 1, 0));

		glBegin(GL_QUADS);
		for (int d = 0; d < 6; d++) {
			vec3i8 dir = DIRS[d];
			glColor3f(playerColor[0], playerColor[1], playerColor[2]);

			glNormal3f(dir[0], dir[1], dir[2]);
			for (int j = 0; j < 4; j++) {
				vec3i off(
					(DIR_QUAD_CORNER_CYCLES_3D[d][j][0] * 2 - 1) * Player::RADIUS,
					(DIR_QUAD_CORNER_CYCLES_3D[d][j][1] * 2 - 1) * Player::RADIUS,
					DIR_QUAD_CORNER_CYCLES_3D[d][j][2] * Player::HEIGHT - Player::EYE_HEIGHT
				);
				glTexCoord2f(QUAD_CORNER_CYCLE[j][0], QUAD_CORNER_CYCLE[j][1]);
				vec3f vertex = off.cast<float>() * (1.0f / RESOLUTION);
				glVertex3f(vertex[0], vertex[1], vertex[2]);
			}
		}
		glEnd();
		LOG_OPENGL_ERROR;
		GL(PopMatrix());
	}
}
