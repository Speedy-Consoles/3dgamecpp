#ifndef GL3_HUD_RENDERER_HPP_
#define GL3_HUD_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "client/gfx/graphics.hpp"
#include "client/client.hpp"
#include "client/gfx/component_renderer.hpp"

class GL3Renderer;
class ShaderManager;

class GL3HudRenderer : public ComponentRenderer {
public:
	GL3HudRenderer(Client *client, GL3Renderer *renderer);

	void render() override;

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;

	GLuint vao;
	GLuint vbo;
};

#endif //GL3_HUD_RENDERER_HPP_
