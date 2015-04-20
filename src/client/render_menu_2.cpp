/*
 * render_menu->cpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#include "menu.hpp"
#include "stopwatch.hpp"

#include "gui/widget.hpp"
#include "gui/frame.hpp"
#include "gui/label.hpp"
#include "gui/button.hpp"

#include <stack>

#include "gl2_renderer.hpp"

using namespace gui;
using namespace std;

void renderFrame(const Frame *frame);
void renderLabel(const Label *label);

void GL2Renderer::renderMenu() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);

	switchToOrthogonal();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(-graphics->getDrawWidth() / 2, -graphics->getDrawHeight() / 2, 0);
	glPushMatrix();
	glTranslatef(0, graphics->getDrawHeight(), 0);

	glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
	glBegin(GL_QUADS);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(0.0f, -graphics->getDrawHeight());
		glVertex2f(graphics->getDrawWidth(), -graphics->getDrawHeight());
		glVertex2f(graphics->getDrawWidth(), 0.0f);
	glEnd();
	glPopMatrix();

	renderWidget((const Widget *) client->getMenu()->getFrame());

	glPopMatrix();
}

void GL2Renderer::renderWidget(const Widget *widget) {
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

void GL2Renderer::renderFrame(const Frame *frame) {
	for (const Widget *widget : frame->widgets()) {
		renderWidget(widget);
	}
}

void GL2Renderer::renderLabel(const Label *label) {
	glColor3f(1.0f, 1.0f, 1.0f);
	renderText(label->text().c_str());
}

void GL2Renderer::renderButton(const Button *button) {
	if (button->hover())
		glColor3f(1.0f, 1.0f, 0.0f);
	else
		glColor3f(1.0f, 1.0f, 1.0f);

	renderText(button->text().c_str());
}

void GL2Renderer::renderText(const char *text) {
	glTranslatef(0, -font->Descender(), 0);
	font->Render(text);
}
