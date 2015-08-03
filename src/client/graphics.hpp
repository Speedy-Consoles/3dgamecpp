#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <memory>

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
	std::unique_ptr<Renderer> renderer;

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
	Graphics(Client *client, const Client::State *state);
	~Graphics();

	bool createContext();
	bool createGL2Context();
	bool createGL3Context();

	void resize(int width, int height);
	void setConf(const GraphicsConf &, const GraphicsConf &);

	int getHeight() const;
	int getWidth() const;

	int getDrawHeight() const;
	int getDrawWidth() const;

	float getScalingFactor() const;

	void tick();
	void flip();

private:
	void calcDrawArea();
	void setMenu(bool menuActive);
};

#endif // GRAPHICS_HPP
