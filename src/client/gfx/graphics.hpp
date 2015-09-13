#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <memory>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "shared/game/world.hpp"
#include "shared/engine/stopwatch.hpp"

#include "client/client.hpp"
#include "client/menu.hpp"
#include "client/config.hpp"

#include "renderer.hpp"

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

	bool isMouseGrabbed = false;

	int width;
	int height;

	float drawWidth;
	float drawHeight;

	// for saving mouse position in menu
	float oldRelMouseX = 0.5;
	float oldRelMouseY = 0.5;

public:
	Graphics(Client *client);
	~Graphics();

	bool createContext();
	void flip();
	void grabMouse(bool);

	void resize(int width, int height);
	void setConf(const GraphicsConf &, const GraphicsConf &);

	int getHeight() const;
	int getWidth() const;

	float getDrawHeight() const;
	float getDrawWidth() const;

	float getScalingFactor() const;

private:
	void calcDrawArea();
};

#endif // GRAPHICS_HPP
