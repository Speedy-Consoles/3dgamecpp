#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "game/world.hpp"
#include "stopwatch.hpp"
#include "menu.hpp"
#include "client.hpp"
#include "renderer.hpp"
#include "config.hpp"

namespace gui {
	class Frame;
	class Label;
	class Widget;
	class Button;
}

class Graphics {
private:
	Client *client = nullptr;

	SDL_GLContext glContext;
	SDL_Window *window;
	Renderer *renderer;
	GraphicsConf conf;

	const Client::State &state;
	Client::State oldState;

	int width;
	int height;

	double drawWidth;
	double drawHeight;

	// for saving mouse position in menu
	double oldRelMouseX = 0.5;
	double oldRelMouseY = 0.5;

public:
	Graphics(Client *client, World *world, const Menu *menu, const Client::State *state,
				const uint8 *localClientId, const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
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

	int getDrawHeight() const;
	int getDrawWidth() const;

	float getScalingFactor() const;

private:
	void calcDrawArea();
	void setMenu(bool menuActive);
};

#endif // GRAPHICS_HPP
