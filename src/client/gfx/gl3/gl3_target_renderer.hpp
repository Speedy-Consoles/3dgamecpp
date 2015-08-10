#ifndef GL3_TARGET_RENDERER_HPP_
#define GL3_TARGET_RENDERER_HPP_

#include "client/client.hpp"
#include "client/graphics.hpp"
#include "engine/macros.hpp"

class GL3Renderer;
class ShaderManager;

class GL3TargetRenderer {
public:
	GL3TargetRenderer(Client *client, GL3Renderer *renderer, ShaderManager *shaderManager);
	~GL3TargetRenderer();

	void render();

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;
	ShaderManager *shaderManager = nullptr;

	vec3f targetColor{ 0.0f, 0.0f, 0.0f };

	GLuint vao;
	GLuint vbo;

	PACKED(
	struct VertexData {
		GLfloat xyz[3];
		GLfloat rgba[4];
	});
};

#endif //GL3_TARGET_RENDERER_HPP_
