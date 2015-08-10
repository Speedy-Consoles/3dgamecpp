#ifndef GL2_SKY_RENDERER_HPP_
#define GL2_SKY_RENDERER_HPP_

#include "engine/vmath.hpp"

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2SkyRenderer {
	Client *client;
	GL2Renderer *renderer;

public:
	GL2SkyRenderer(Client *client, GL2Renderer *renderer);
	~GL2SkyRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

private:
	const vec3f fogColor{ 0.6f, 0.6f, 0.8f };
	const vec3f skyColor{ 0.15f, 0.15f, 0.9f };
};

#endif //GL2_SKY_RENDERER_HPP_
