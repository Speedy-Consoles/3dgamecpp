#include "graphics.hpp"

#include <SDL2/SDL_image.h>

#include "engine/logging.hpp"
#include "gl2_renderer.hpp"
#include "gl3_renderer.hpp"

using namespace gui;

Graphics::Graphics(
		Client *client,
		World *world,
		const Menu *menu,
		const Client::State *state,
		const uint8 *localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch)
		:
		client(client),
		state(*state)
{
	LOG(DEBUG, "Constructing Graphics");

	LOG(DEBUG, "Initializing SDL");
	if (SDL_Init(SDL_INIT_VIDEO))
		LOG(FATAL, SDL_GetError());
	int img_init_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	int img_init_result = IMG_Init(img_init_flags);
	if (!(img_init_flags & img_init_result & IMG_INIT_JPG)) {
		LOG(ERROR, "SDL_Image JPEG Plugin could not be initialized")
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_PNG)) {
		LOG(ERROR, "SDL_Image PNG Plugin could not be initialized")
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_TIF)) {
		LOG(ERROR, "SDL_Image TIFF Plugin could not be initialized")
	}

	LOG(DEBUG, "Creating window");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetSwapInterval(0);
	SDL_SetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");
	window = SDL_CreateWindow(
		"3dgame",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		conf.windowed_res[0], conf.windowed_res[1],
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (!window) LOG(FATAL, SDL_GetError());

	width = conf.windowed_res[0];
	height = conf.windowed_res[1];
	calcDrawArea();
	glViewport(0, 0, width, height);

	LOG(DEBUG, "Creating Open GL Context");
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) LOG(FATAL, SDL_GetError());

	LOG(INFO, "OpenGL Version: " << glGetString(GL_VERSION));
	LOG(INFO, "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION));

	LOG(DEBUG, "Initializing GLEW");
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
		LOG(FATAL, glewGetErrorString(glew_error));
	if (!GLEW_VERSION_2_1)
		LOG(FATAL, "OpenGL version 2.1 not available");
	if (GLEW_VERSION_3_3 && conf.render_backend == RenderBackend::OGL_3) {
		renderer = new GL3Renderer(client, this);
	} else {
		renderer = new GL2Renderer(client, this);
	}
}

Graphics::~Graphics() {
	delete renderer;

	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

void Graphics::resize(int width, int height) {
	LOG(INFO, "Resize to " << width << "x" << height);
	this->width = width;
	this->height = height;
	calcDrawArea();
	glViewport(0, 0, width, height);
	renderer->resize();
}

void Graphics::calcDrawArea() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = width / (double) height;
	if (currentRatio > normalRatio) {
		drawWidth = DEFAULT_WINDOWED_RES[0];
		drawHeight = DEFAULT_WINDOWED_RES[0] / currentRatio;
	} else {
		drawWidth = DEFAULT_WINDOWED_RES[1] * currentRatio;
		drawHeight = DEFAULT_WINDOWED_RES[1];
	}
}

void Graphics::setMenu(bool menuActive) {
	SDL_SetWindowGrab(window, (SDL_bool) !menuActive);
	SDL_SetRelativeMouseMode((SDL_bool) !menuActive);
	if (menuActive) {
		SDL_WarpMouseInWindow(window, (int) (oldRelMouseX * width), (int) (oldRelMouseY * height));
	} else {
		int x = width / 2;
		int y = height / 2;
		SDL_GetMouseState(&x, &y);
		oldRelMouseX = x / (double) width;
		oldRelMouseY = y / (double) height;
	}
}

int Graphics::getWidth() const {
	return width;
}

int Graphics::getHeight() const {
	return height;
}

int Graphics::getDrawWidth() const {
	return drawWidth;
}

int Graphics::getDrawHeight() const {
	return drawHeight;
}

float Graphics::getScalingFactor() const {
	return (float) drawWidth / width;
}

void Graphics::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.fullscreen != old.fullscreen) {
		SDL_SetWindowFullscreen(window, conf.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
	}

	renderer->setConf(conf, old);
}

void Graphics::tick() {
	if (oldState != state) {
		if (state == Client::State::IN_MENU)
			setMenu(true);
		else if (oldState == Client::State::IN_MENU)
			setMenu(false);
		oldState = state;
	}
	renderer->tick();
}

void Graphics::flip() {
	SDL_GL_SwapWindow(window);
}