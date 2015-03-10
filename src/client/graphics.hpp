#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "gl_20_renderer.hpp"
#include "config.hpp"

namespace gui {
	class Frame;
	class Label;
	class Widget;
	class Button;
}

class Graphics {
private:
	SDL_GLContext glContext;
	SDL_Window *window;
	GL20Renderer *renderer;
	GraphicsConf conf;
public:
	Graphics(World *world, const Menu *menu, const ClientState *state, const uint8 *localClientId, const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
	~Graphics();

	void tick();

	void resize(int width, int height);

	void setDebug(bool debugActive);
	bool isDebug();

	bool getCloseRequested();

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);

	int getHeight() const;
	int getWidth() const;

	float getScalingFactor() const;
};

#endif // GRAPHICS_HPP
