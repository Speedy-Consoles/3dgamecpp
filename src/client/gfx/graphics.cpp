#include "graphics.hpp"

#include <SDL2/SDL_image.h>

#include "shared/engine/logging.hpp"

#include "gl2/gl2_renderer.hpp"
#include "gl3/gl3_renderer.hpp"

using namespace gui;

static logging::Logger logger("gfx");

Graphics::Graphics(Client *client, const Client::State *state) : client(client), state(*state) {
	LOG_DEBUG(logger) << "Constructing Graphics";

	LOG_DEBUG(logger) << "Initializing SDL";
	if (SDL_Init(SDL_INIT_VIDEO))
		LOG_FATAL(logger) << SDL_GetError();
	int img_init_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	int img_init_result = IMG_Init(img_init_flags);
	if (!(img_init_flags & img_init_result & IMG_INIT_JPG)) {
		LOG_ERROR(logger) << "SDL_Image JPEG Plugin could not be initialized";
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_PNG)) {
		LOG_ERROR(logger) << "SDL_Image PNG Plugin could not be initialized";
	}
	if (!(img_init_flags & img_init_result & IMG_INIT_TIF)) {
		LOG_ERROR(logger) << "SDL_Image TIFF Plugin could not be initialized";
	}
}

Graphics::~Graphics() {
	renderer.reset();

	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(glContext);
	SDL_Quit();
}

bool Graphics::createContext() {
	LOG_DEBUG(logger) << "Creating window";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetSwapInterval(0);
	SDL_SetHint("SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");
	window = SDL_CreateWindow(
		"3dgame",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		client->getConf().windowed_res[0], client->getConf().windowed_res[1],
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	if (!window)
		LOG_FATAL(logger) << SDL_GetError();

	width = client->getConf().windowed_res[0];
	height = client->getConf().windowed_res[1];
	calcDrawArea();
	glViewport(0, 0, width, height);

	LOG_DEBUG(logger) << "Creating Open GL Context";
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) LOG_FATAL(logger) << SDL_GetError();

	LOG_INFO(logger) << "OpenGL Version: " << glGetString(GL_VERSION);
	LOG_INFO(logger) << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);

	LOG_DEBUG(logger) << "Initializing GLEW";
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK) {
		LOG_FATAL(logger) << glewGetErrorString(glew_error);
		return false;
	}
	if (!GLEW_VERSION_2_1) {
		LOG_FATAL(logger) << "OpenGL version 2.1 not available";
		return false;
	}
	if (GLEW_VERSION_3_3 && client->getConf().render_backend == RenderBackend::OGL_3) {
		renderer = std::unique_ptr<GL3Renderer>(new GL3Renderer(client));
	} else {
		renderer = std::unique_ptr<GL2Renderer>(new GL2Renderer(client));
	}
	return true;
}

void Graphics::resize(int width, int height) {
	LOG_INFO(logger) << "Resize to " << width << "x" << height;
	this->width = width;
	this->height = height;
	calcDrawArea();
	glViewport(0, 0, width, height);
	renderer->resize();
}

void Graphics::calcDrawArea() {
	float normalRatio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float currentRatio = (float) width / height;
	if (currentRatio > normalRatio) {
		drawWidth = DEFAULT_WINDOWED_RES[0] * 1.0f;
		drawHeight = DEFAULT_WINDOWED_RES[0] / currentRatio;
	} else {
		drawWidth = DEFAULT_WINDOWED_RES[1] * currentRatio;
		drawHeight = DEFAULT_WINDOWED_RES[1] * 1.0f;
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
		oldRelMouseX = x / (float) width;
		oldRelMouseY = y / (float) height;
	}
}

int Graphics::getWidth() const {
	return width;
}

int Graphics::getHeight() const {
	return height;
}

float Graphics::getDrawWidth() const {
	return drawWidth;
}

float Graphics::getDrawHeight() const {
	return drawHeight;
}

float Graphics::getScalingFactor() const {
	return drawWidth / width;
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
	renderer->render();
}

void Graphics::flip() {
	SDL_GL_SwapWindow(window);
}
