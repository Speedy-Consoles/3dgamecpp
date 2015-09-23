#ifndef WITHOUT_GL2

#include "gl2_menu_renderer.hpp"

#include "shared/engine/logging.hpp"

#include "client/gui/label.hpp"
#include "client/gui/button.hpp"
#include "client/gui/cycle_button.hpp"

#include "gl2_renderer.hpp"

using namespace gui;

static logging::Logger logger("render");

GL2MenuRenderer::GL2MenuRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
	if (font) {
		font->FaceSize(16);
		font->CharMap(ft_encoding_unicode);
	} else {
		LOG_ERROR(logger) << "Could not open 'res/DejaVuSansMono.ttf'";
	}
}

GL2MenuRenderer::~GL2MenuRenderer() {
	delete font;
}

void GL2MenuRenderer::render() {
	if (client->getStateId() != Client::StateId::MENU)
		return;

	auto w = client->getGraphics()->getDrawWidth();
	auto h = client->getGraphics()->getDrawHeight();

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(-w / 2.0f, -h / 2.0f, 0.0f);
	glTranslatef(0.0f, (float) h, 0.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
	glBegin(GL_QUADS);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(0.0f, (float) -h);
		glVertex2f((float) w, (float) -h);
		glVertex2f((float) w, 0.0f);
	glEnd();
	glPopMatrix();

	renderWidget((const Widget *) client->getMenu()->getFrame());
}

void GL2MenuRenderer::renderWidget(const Widget *widget) {
	glPushMatrix();
	glTranslatef(widget->x(), widget->y(), 0);

	const Label *label = nullptr;
	const Button *button = nullptr;

	if ((button = dynamic_cast<const Button *>(widget))) {
		renderButton(button);
	} else if ((label = dynamic_cast<const Label *>(widget))) {
		renderLabel(label);
	}

	glPopMatrix();

	for (const Widget *child : widget->children()) {
		renderWidget(child);
	}
}

void GL2MenuRenderer::renderLabel(const Label *label) {
	glColor3f(1.0f, 1.0f, 1.0f);
	renderText(label->text().c_str());
}

void GL2MenuRenderer::renderButton(const Button *button) {
	if (button->hover())
		glColor3f(1.0f, 1.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);

	renderText(button->text().c_str());
}

void GL2MenuRenderer::renderText(const char *text) {
	glTranslatef(0, -font->Descender(), 0);
	font->Render(text);
}

#endif // WITHOUT_GL2
