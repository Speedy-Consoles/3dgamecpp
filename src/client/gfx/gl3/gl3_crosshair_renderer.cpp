#include "gl3_crosshair_renderer.hpp"

#define GLM_FORCE_RADIANS

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"

#include "gl3_renderer.hpp"

GL3CrosshairRenderer::GL3CrosshairRenderer(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer)
{
	GL(GenVertexArrays(1, &vao));
	GL(GenBuffers(1, &vbo));
	GL(BindVertexArray(vao));
	GL(BindBuffer(GL_ARRAY_BUFFER, vbo));

	PACKED(
	struct VertexData {
		GLfloat xy[2];
		GLfloat rgba[4];
	});

	VertexData vertexData[12] = {
			// x        y       r     g     b     a
			{{-20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ 20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{-20.0f,   2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{-20.0f,  -2.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{  2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f,  20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
			{{ -2.0f, -20.0f}, {0.0f, 0.0f, 0.0f, 0.5f}},
	};

	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) 0));
	GL(VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) 8));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
	GL(BindVertexArray(0));
}

void GL3CrosshairRenderer::render() {
	if (client->getState() == Client::State::PLAYING) {
		HudShader &shader = ((GL3Renderer *) renderer)->getShaderManager()->getHudShader();
		shader.setModelMatrix(glm::mat4(1.0f));
		shader.setColor(glm::vec4{ 1, 1, 1, 1 });
		shader.useProgram();
		GL(BindVertexArray(vao));
		GL(DrawArrays(GL_TRIANGLES, 0, 12));
	}
}
