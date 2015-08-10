#ifndef GL2_HUD_RENDERER_HPP_
#define GL2_HUD_RENDERER_HPP_

class Client;
class GL2Renderer;
struct GraphicsConf;

class GL2HudRenderer {
	Client *client;
	GL2Renderer *renderer;

public:
	GL2HudRenderer(Client *client, GL2Renderer *renderer);
	~GL2HudRenderer();

	void setConf(const GraphicsConf &, const GraphicsConf &);
	void render();

private:
	void renderCrosshair();
	void renderSelectedBlock();
};

#endif //GL2_HUD_RENDERER_HPP_
