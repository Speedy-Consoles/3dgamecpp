#include "gl2_sky_renderer.hpp"

#include "gl2_renderer.hpp"

GL2SkyRenderer::GL2SkyRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	// nothing
}

void GL2SkyRenderer::render() {
	glBegin(GL_QUADS);
	glColor3fv(fogColor.ptr());
	glVertex3d(-2, -2, 2);
	glVertex3d(2, -2, 2);
	glVertex3d(2, 0.3, -1);
	glVertex3d(-2, 0.3, -1);

	glVertex3d(-2, 0.3, -1);
	glVertex3d(2, 0.3, -1);
	glColor3fv(skyColor.ptr());
	glVertex3d(2, 2, 2);
	glVertex3d(-2, 2, 2);
	glEnd();
}
