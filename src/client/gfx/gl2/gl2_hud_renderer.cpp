#ifndef WITHOUT_GL2

#include "gl2_hud_renderer.hpp"

#include "shared/engine/logging.hpp"

#include "gl2_renderer.hpp"

GL2HudRenderer::GL2HudRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	// nothing
}

void GL2HudRenderer::render() {
	if (client->getStateId() != Client::StateId::PLAYING)
		return;

	const Character &character = client->getLocalCharacter();
	if (!character.isValid())
		return;

	GL(Enable(GL_TEXTURE_2D));
	vec2f texs[4];
	GL2TextureManager::Entry tex_entry = renderer->getTextureManager()->get(client->getLocalCharacter().getBlock());
	GL2TextureManager::getTextureCoords(tex_entry.index, tex_entry.type, texs);
	GL(BindTexture(GL_TEXTURE_2D, tex_entry.tex));

	GL(Color4f(1.0f, 1.0f, 1.0f, 1.0f));

	GL(PushMatrix());
	float d = (client->getGraphics()->getWidth() < client->getGraphics()->getHeight() ? client->getGraphics()->getWidth() : client->getGraphics()->getHeight()) * 0.05f;
	GL(Translatef(-client->getGraphics()->getDrawWidth() * 0.48f, -client->getGraphics()->getDrawHeight() * 0.48f, 0));
	glBegin(GL_QUADS);
		glTexCoord2f(texs[0][0], texs[0][1]); glVertex2f(0, 0);
		glTexCoord2f(texs[1][0], texs[1][1]); glVertex2f(d, 0);
		glTexCoord2f(texs[2][0], texs[2][1]); glVertex2f(d, d);
		glTexCoord2f(texs[3][0], texs[3][1]); glVertex2f(0, d);
	glEnd();
	LOG_OPENGL_ERROR;

	GL(PopMatrix());
}

#endif // WITHOUT_GL2
