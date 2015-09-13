#include "gl3_sky_renderer.hpp"

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "client/gfx/component_renderer.hpp"

#include "gl3_renderer.hpp"

GL3SkyRenderer::GL3SkyRenderer(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	GL(GenVertexArrays(1, &vao));
	GL(GenBuffers(1, &vbo));
	GL(BindVertexArray(vao));
	GL(BindBuffer(GL_ARRAY_BUFFER, vbo));

	VertexData vertexData[12] = {
			// x      y      z       r            g            b            a
			{{-2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{-2.0f, -2.0f,  2.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},

			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
			{{ 2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{ 2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{-2.0f,  2.0f,  2.0f}, {skyColor[0], skyColor[1], skyColor[2], 1.0f}},
			{{-2.0f,  0.3f, -1.0f}, {fogColor[0], fogColor[1], fogColor[2], 1.0f}},
	};

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, 0));
	GL(VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 28, (void *) 12));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
	GL(BindVertexArray(0));
}

void GL3SkyRenderer::render() {
	Player &player = client->getLocalPlayer();
	if (!player.isValid())
		return;

	auto &defaultShader = ((GL3Renderer *) renderer)->getShaderManager()->getDefaultShader();
	glm::mat4 viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 36000.0f * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
	defaultShader.setViewMatrix(viewMatrix);
	defaultShader.setModelMatrix(glm::mat4(1.0f));
	defaultShader.setFogEnabled(false);
    defaultShader.setLightEnabled(false);
	defaultShader.useProgram();

	GL(BindVertexArray(vao));
	GL(DrawArrays(GL_TRIANGLES, 0, 12));
}
