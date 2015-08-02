#include "gl2_menu_renderer.hpp"

#include "gl2_renderer.hpp"

#include "gui/frame.hpp"
#include "gui/label.hpp"
#include "gui/button.hpp"
#include "gui/cycle_button.hpp"

#include "engine/logging.hpp"

using namespace gui;

GL2MenuRenderer::GL2MenuRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
	if (font) {
		font->FaceSize(16);
		font->CharMap(ft_encoding_unicode);
	} else {
		LOG(ERROR, "Could not open 'res/DejaVuSansMono.ttf'");
	}
}

GL2MenuRenderer::~GL2MenuRenderer() {
	delete font;
}

void GL2MenuRenderer::setConf(const GraphicsConf &, const GraphicsConf &) {

}

void GL2MenuRenderer::render() {
	auto w = client->getGraphics()->getDrawWidth();
	auto h = client->getGraphics()->getDrawHeight();

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(-w / 2, -h / 2, 0);

	glPushMatrix();
	glTranslatef(0, h, 0);
	glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
	glBegin(GL_QUADS);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(0.0f, -h);
		glVertex2f(w, -h);
		glVertex2f(w, 0.0f);
	glEnd();
	glPopMatrix();

	renderWidget((const Widget *) client->getMenu()->getFrame());

	glPopMatrix();
}

void GL2MenuRenderer::renderWidget(const Widget *widget) {
	glPushMatrix();
	glTranslatef(widget->x(), widget->y(), 0);

	const Label *label = nullptr;
	const Frame *frame = nullptr;
	const Button *button = nullptr;

	if ((button = dynamic_cast<const Button *>(widget))) {
		renderButton(button);
	} else if ((label = dynamic_cast<const Label *>(widget))) {
		renderLabel(label);
	} else if ((frame = dynamic_cast<const Frame *>(widget))) {
		renderFrame(frame);
	}

	glPopMatrix();
}

void GL2MenuRenderer::renderFrame(const Frame *frame) {
	for (const Widget *widget : frame->widgets()) {
		renderWidget(widget);
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