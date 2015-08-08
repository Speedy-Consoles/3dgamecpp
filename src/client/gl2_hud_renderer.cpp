#include "gl2_hud_renderer.hpp"

#include "gl2_renderer.hpp"
#include "engine/logging.hpp"

GL2HudRenderer::GL2HudRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	// nothing
}

GL2HudRenderer::~GL2HudRenderer() {

}

void GL2HudRenderer::setConf(const GraphicsConf &, const GraphicsConf &) {

}

void GL2HudRenderer::render() {
	renderCrosshair();
	renderSelectedBlock();
}

void GL2HudRenderer::renderCrosshair() {
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

void GL2HudRenderer::renderSelectedBlock() {
	GL(Enable(GL_TEXTURE_2D));
	vec2f texs[4];
	TextureManager::Entry tex_entry = renderer->getTextureManager()->get(client->getLocalPlayer().getBlock());
	TextureManager::getVertices(tex_entry, texs);
	GL(BindTexture(GL_TEXTURE_2D, tex_entry.tex));

	GL(Color4f(1, 1, 1, 1));

	GL(PushMatrix());
	float d = (client->getGraphics()->getWidth() < client->getGraphics()->getHeight() ? client->getGraphics()->getWidth() : client->getGraphics()->getHeight()) * 0.05;
	GL(Translatef(-client->getGraphics()->getDrawWidth() * 0.48, -client->getGraphics()->getDrawHeight() * 0.48, 0));
	glBegin(GL_QUADS);
		glTexCoord2f(texs[0][0], texs[0][1]); glVertex2f(0, 0);
		glTexCoord2f(texs[1][0], texs[1][1]); glVertex2f(d, 0);
		glTexCoord2f(texs[2][0], texs[2][1]); glVertex2f(d, d);
		glTexCoord2f(texs[3][0], texs[3][1]); glVertex2f(0, d);
	glEnd();
	LOG_OPENGL_ERROR;

	GL(PopMatrix());
}
