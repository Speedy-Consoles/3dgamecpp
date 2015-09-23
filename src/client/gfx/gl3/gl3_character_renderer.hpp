#ifndef GL3_CHARACTER_RENDERER_HPP_
#define GL3_CHARACTER_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "shared/engine/vmath.hpp"
#include "shared/game/character.hpp"
#include "client/client.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gfx/component_renderer.hpp"

class GL3Renderer;
class ShaderManager;

class GL3CharacterRenderer : public ComponentRenderer {
	static const vec3f CHARACTER_COLOR;
	static const vec3i HEAD_SIZE;
	static const vec3i BODY_SIZE;
	static const int HEAD_Z_OFFSET = 100;
	static const int HEAD_ANCHOR_Z_OFFSET = -100;
	static const int PITCH_MIN = -60;
	static const int PITCH_MAX = 60;
private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;

	GLuint bodyVao;
	GLuint bodyVbo;
	GLuint headVao;
	GLuint headVbo;

	PACKED(
	struct VertexData {
		GLfloat xyz[3];
		GLfloat nxyz[3];
		GLfloat rgba[4];
	});
public:
	GL3CharacterRenderer(Client *client, GL3Renderer *renderer);
	~GL3CharacterRenderer();

	void render() override;
private:
	void buildBody();
	void buildHead();
};

#endif //GL3_CHARACTER_RENDERER_HPP_
