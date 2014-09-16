/*
 * render_menu.cpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#include "graphics.hpp"

void Graphics::renderMenu() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);

	glClearColor(0.2f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	switchToOrthogonal();
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glLoadIdentity();
	glTranslatef(-drawWidth / 2 + 3, drawHeight / 2, 0);

	char buffer[1024];
	#define RENDER_LINE(args...) sprintf(buffer, args);\
			glTranslatef(0, -16, 0);\
			font->Render(buffer)

	glColor3f(1.0f, 1.0f, 1.0f);

	for (uint i = 0; i < menu.getEntryCount(); ++i) {
		std::string name = menu.getEntryName(i);
		std::string value = menu.getEntryValue(i);

		bool is_highlighted = i == menu.getCursor();

		if (is_highlighted)
			glColor3f(1.0f, 1.0f, 0.0f);
		else
			glColor3f(0.8f, 0.8f, 0.8f);

		RENDER_LINE("%s: %s", name.c_str(), value.c_str());
	}

	glPopMatrix();
}
