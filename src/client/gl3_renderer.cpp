#include "gl3_renderer.hpp"

#include "engine/logging.hpp"

#include "menu.hpp"
#include "stopwatch.hpp"

#include "engine/math.hpp"

#include "game/world.hpp"

using namespace gui;

GL3Renderer::GL3Renderer(
		Graphics *graphics,
		SDL_Window *window,
		World *world,
		const Menu *menu,
		const ClientState *state,
		const uint8 *localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		graphics(graphics),
		conf(conf),
		window(window),
		world(world),
		menu(menu),
		localClientID(*localClientID),
		stopwatch(stopwatch) {
	// TODO
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying Graphics");
	// TODO
}
void GL3Renderer::resize(int width, int height) {
	LOG(INFO, "Resize to " << width << "x" << height);
	// TODO
}

void GL3Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::setDebug(bool debugActive) {
	// TODO
}

bool GL3Renderer::isDebug() {
	// TODO
	return false;
}

void GL3Renderer::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;
	// TODO
}

void GL3Renderer::tick() {
	// TODO
}
