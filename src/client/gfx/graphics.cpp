#include "graphics.hpp"

#include <SDL2/SDL_image.h>

#include "shared/engine/logging.hpp"

#include "gl2/gl2_renderer.hpp"
#include "gl3/gl3_renderer.hpp"

using namespace gui;

static logging::Logger logger("gfx");

Graphics::Graphics(Client *client) : client(client) {
	LOG_DEBUG(logger) << "Constructing Graphics";

	LOG_DEBUG(logger) << "Initializing SDL";
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
		LOG_FATAL(logger) << SDL_GetError();
	int img_init_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	int img_init_result = IMG_Init(img_init_flags);
	if ((img_init_flags & IMG_INIT_JPG) != 0 && (img_init_result & IMG_INIT_JPG) == 0) {
		LOG_ERROR(logger) << "SDL_Image JPEG Plugin could not be initialized";
	}
	if ((img_init_flags & IMG_INIT_PNG) != 0 && (img_init_result & IMG_INIT_PNG) == 0) {
		LOG_ERROR(logger) << "SDL_Image PNG Plugin could not be initialized";
	}
	if ((img_init_flags & IMG_INIT_TIF) != 0 && (img_init_result & IMG_INIT_TIF) == 0) {
		LOG_ERROR(logger) << "SDL_Image TIFF Plugin could not be initialized";
	}
}

Graphics::~Graphics() {
	LOG_TRACE(logger) << "Destroying Graphics";
	IMG_Quit();
	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(glContext);
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
	return true;
}

void Graphics::flip() {
	SDL_GL_SwapWindow(window);
}

void Graphics::grabMouse(bool b) {
	if (isMouseGrabbed == b)
		return;

	SDL_SetWindowGrab(window, b ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(b ? SDL_TRUE : SDL_FALSE);
	if (b) {
		int x = width / 2;
		int y = height / 2;
		SDL_GetMouseState(&x, &y);
		oldRelMouseX = x / (float) width;
		oldRelMouseY = y / (float) height;
	} else {
		SDL_WarpMouseInWindow(window, (int) (oldRelMouseX * width), (int) (oldRelMouseY * height));
	}

	isMouseGrabbed = b;
}

void Graphics::resize(int width, int height) {
	LOG_INFO(logger) << "Resize to " << width << "x" << height;
	this->width = width;
	this->height = height;
	calcDrawArea();
	glViewport(0, 0, width, height);
}

void Graphics::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.fullscreen != old.fullscreen) {
		SDL_SetWindowFullscreen(window, conf.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
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
