#ifndef WITHOUT_GL2

#ifndef GL2_DEBUG_RENDERER_HPP_
#define GL2_DEBUG_RENDERER_HPP_

#include <FTGL/ftgl.h>

#include "client/gfx/component_renderer.hpp"

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2DebugRenderer : public ComponentRenderer {
	Client *client = nullptr;
	GL2Renderer *renderer = nullptr;

	FTFont *font = nullptr;

public:
	GL2DebugRenderer(Client *client, GL2Renderer *renderer);
	~GL2DebugRenderer();
	
	void render() override;

private:
	void renderDebug();
	void renderPerformance();
};

#endif //GL2_DEBUG_RENDERER_HPP_

#endif // WITHOUT_GL2
