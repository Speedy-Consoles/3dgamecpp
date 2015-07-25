#include "gl3_hud_renderer.hpp"

#include "client/gl3_renderer.hpp"

#include "engine/logging.hpp"

GL3HudRenderer::GL3HudRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager, Graphics *graphics) :
	client(client),
	renderer(renderer),
	shaderManager(shaderManager),
	graphics(graphics)
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) 8);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GL3HudRenderer::render() {
	shaderManager->getHudShader().useProgram();
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 12);
}
