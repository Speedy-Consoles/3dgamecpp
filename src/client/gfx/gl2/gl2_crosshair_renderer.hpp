#ifndef WITHOUT_GL2

#ifndef GL2_CROSSHAIR_RENDERER_HPP_
#define GL2_CROSSHAIR_RENDERER_HPP_

#include "client/gfx/component_renderer.hpp"

class Client;
class GL2Renderer;

class GL2CrosshairRenderer : public ComponentRenderer {
	Client *client;
	GL2Renderer *renderer;

public:
	GL2CrosshairRenderer(Client *client, GL2Renderer *renderer);

	void render() override;
};

#endif //GL2_CROSSHAIR_RENDERER_HPP_

#endif // WITHOUT_GL2
