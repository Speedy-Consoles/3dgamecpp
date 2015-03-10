#ifndef GL3_RENDERER_HPP
#define GL3_RENDERER_HPP

#include "renderer.hpp"

#include "client.hpp"
#include "config.hpp"

class Graphics;
class SDL_Window;
class World;
class Player;
class Menu;
class Stopwatch;

namespace gui {
	class Frame;
	class Label;
	class Widget;
	class Button;
}

class GL3Renderer : public Renderer {
private:
	Graphics *graphics;
	GraphicsConf conf;

	SDL_Window *window;
	World *world;
	const Menu *menu;
	const uint8 &localClientID;
	Stopwatch *stopwatch;

	double maxFOV;

public:
	GL3Renderer(Graphics *graphics, SDL_Window *window, World *world, const Menu *menu,
				const ClientState *state, const uint8 *localClientId,
				const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
	~GL3Renderer();

	void tick();

	void resize(int width, int height);
	void makeMaxFOV();

	void setDebug(bool debugActive);
	bool isDebug();

	bool getCloseRequested();

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);
};

#endif // GL_3_RENDERER_HPP
