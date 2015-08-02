#ifndef GL2_DEBUG_RENDERER_HPP_
#define GL2_DEBUG_RENDERER_HPP_

#include <FTGL/ftgl.h>

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2DebugRenderer {
	Client *client = nullptr;
	GL2Renderer *renderer = nullptr;

	FTFont *font = nullptr;

public:
	GL2DebugRenderer(Client *client, GL2Renderer *renderer);
	~GL2DebugRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

private:
	void renderDebug();
	void renderPerformance();
};

#endif //GL2_DEBUG_RENDERER_HPP_
