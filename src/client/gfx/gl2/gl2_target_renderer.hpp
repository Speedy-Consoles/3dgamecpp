#ifndef WITHOUT_GL2
#ifndef GL2_TARGET_RENDERER_HPP_
#define GL2_TARGET_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "shared/engine/vmath.hpp"
#include "client/gfx/component_renderer.hpp"

class Client;
class GL2Renderer;

class GL2TargetRenderer : public ComponentRenderer {
public:
	GL2TargetRenderer(Client *client, GL2Renderer *renderer);

	void render() override;

private:
	Client *client = nullptr;
	GL2Renderer *renderer = nullptr;

	vec3f targetColor{ 0.0f, 0.0f, 0.0f };
};

#endif //GL2_TARGET_RENDERER_HPP_
#endif // WITHOUT_GL2
