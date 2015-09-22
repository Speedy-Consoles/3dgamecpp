#ifndef GL3_CHARACTER_RENDERER_HPP_
#define GL3_CHARACTER_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "shared/engine/vmath.hpp"
#include "client/client.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gfx/component_renderer.hpp"

class GL3Renderer;
class ShaderManager;

class GL3CharacterRenderer : public ComponentRenderer {
public:
	GL3CharacterRenderer(Client *client, GL3Renderer *renderer);
	~GL3CharacterRenderer();

	void render() override;

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;

	vec3f characterColor{ 0.6f, 0.0f, 0.0f };

	GLuint vao;
	GLuint vbo;

	PACKED(
	struct VertexData {
		GLfloat xyz[3];
		GLfloat rgba[4];
	});
};

#endif //GL3_CHARACTER_RENDERER_HPP_
