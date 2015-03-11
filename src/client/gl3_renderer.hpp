#ifndef GL3_RENDERER_HPP
#define GL3_RENDERER_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "renderer.hpp"

#include "client.hpp"
#include "config.hpp"

class Graphics;
struct SDL_Window;
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

	// 3D values
	float ZNEAR = 0.1f;
	float maxFOV;

	// transformation matrices
	glm::mat4 perspectiveMatrix;
	glm::mat4 orthogonalMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 modelMatrix;

	// program Location
	GLuint progLoc;

	// uniform locations
	GLuint projMatLoc;
	GLuint viewMatLoc;
	GLuint modelMatLoc;

	// This will identify our vertex buffer
	GLuint vertexBuffer;

public:
	GL3Renderer(Graphics *graphics, SDL_Window *window, World *world, const Menu *menu,
				const ClientState *state, const uint8 *localClientId,
				const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
	~GL3Renderer();

	void tick();

	void resize(int width, int height);
	void makePerspectiveMatrix();
	void makeOrthogonalMatrix();
	void makeMaxFOV();

	void setViewMatrix();
	void setModelMatrix();
	void setPerspectiveMatrix();
	void setOrthogonalMatrix();

	void setDebug(bool debugActive);
	bool isDebug();

	bool getCloseRequested();

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);

private:
	void loadShaders();
};

#endif // GL_3_RENDERER_HPP
