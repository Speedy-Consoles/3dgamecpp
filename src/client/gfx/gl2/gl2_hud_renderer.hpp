#ifndef GL2_HUD_RENDERER_HPP_
#define GL2_HUD_RENDERER_HPP_

#include "client/gfx/component_renderer.hpp"

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2HudRenderer : public ComponentRenderer {
	Client *client;
	GL2Renderer *renderer;

public:
	GL2HudRenderer(Client *client, GL2Renderer *renderer);

	void render() override;

private:
	void renderCrosshair();
	void renderSelectedBlock();
};

#endif //GL2_HUD_RENDERER_HPP_
