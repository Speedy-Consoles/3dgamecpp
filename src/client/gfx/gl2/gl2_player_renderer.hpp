#ifndef GL2_PLAYER_RENDERER_HPP_
#define GL2_PLAYER_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "shared/engine/vmath.hpp"
#include "client/gfx/component_renderer.hpp"

class Client;
class GL2Renderer;

class GL2PlayerRenderer : public ComponentRenderer {
public:
	GL2PlayerRenderer(Client *client, GL2Renderer *renderer);

	void render() override;

private:
	Client *client = nullptr;
	GL2Renderer *renderer = nullptr;

	vec3f playerColor{ 0.6f, 0.0f, 0.0f };
};

#endif //GL2_PLAYER_RENDERER_HPP_
