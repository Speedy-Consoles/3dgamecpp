#include "gl2_crosshair_renderer.hpp"

#include "shared/engine/logging.hpp"

#include "gl2_renderer.hpp"

GL2CrosshairRenderer::GL2CrosshairRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	// nothing
}

void GL2CrosshairRenderer::render() {
	if (client->getState() == Client::State::PLAYING) {
		GL(Disable(GL_TEXTURE_2D));

		GL(Color4d(0, 0, 0, 0.5));

		glBegin(GL_QUADS);
			glVertex2d(-20, -2);
			glVertex2d(20, -2);
			glVertex2d(20, 2);
			glVertex2d(-20, 2);

			glVertex2d(-2, -20);
			glVertex2d(2, -20);
			glVertex2d(2, 20);
			glVertex2d(-2, 20);
		glEnd();
		LOG_OPENGL_ERROR;
	}
}
