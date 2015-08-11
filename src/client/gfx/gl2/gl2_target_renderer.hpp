#ifndef GL2_TARGET_RENDERER_HPP_
#define GL2_TARGET_RENDERER_HPP_

#include "shared/engine/macros.hpp"
#include "client/client.hpp"
#include "client/gfx/graphics.hpp"

class GL2Renderer;
class ShaderManager;

class GL2TargetRenderer {
public:
	GL2TargetRenderer(Client *client, GL2Renderer *renderer);
	~GL2TargetRenderer();

	void render();

private:
	Client *client = nullptr;
	GL2Renderer *renderer = nullptr;
	ShaderManager *shaderManager = nullptr;

	vec3f targetColor{ 0.0f, 0.0f, 0.0f };
};

#endif //GL2_TARGET_RENDERER_HPP_
