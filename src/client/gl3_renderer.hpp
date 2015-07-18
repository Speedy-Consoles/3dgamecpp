#ifndef GL3_RENDERER_HPP
#define GL3_RENDERER_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "renderer.hpp"
#include "shaders.hpp"
#include "chunk_renderer.hpp"
#include "gl3_debug_renderer.hpp"
#include "gl3_menu_renderer.hpp"
#include "bmfont.hpp"

#include "game/chunk.hpp"
#include "util.hpp"
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
	Client *client = nullptr;
	Graphics *graphics = nullptr;
	SDL_Window *window = nullptr;
	GraphicsConf conf;

	// 3D values
	float ZNEAR = 0.1f;
	float maxFOV;

	// shaders
	ShaderManager shaderManager;
	Shaders &shaders;

    // font
	BMFont fontTimes;
	BMFont fontDejavu;

	// chunk renderer
	ChunkRenderer chunkRenderer;
	GL3DebugRenderer debugRenderer;
	GL3MenuRenderer menuRenderer;

	// vao, vbo locations
	GLuint crossHairVAO;
	GLuint crossHairVBO;
	GLuint skyVAO;
	GLuint skyVBO;

	// scene fbo
	GLuint skyFBO;
	GLuint skyTexture;

	// performance info
	int prevFPS[20];
	int fpsCounter = 0;
	int fpsSum = 0;
	size_t fpsIndex = 0;
	time_t lastFPSUpdate = 0;
	time_t lastStopWatchSave = 0;

	// light
	glm::vec3 ambientColor = glm::vec3(0.4f, 0.4f, 0.35f);
	glm::vec3 diffuseDirection = glm::vec3(1.0f, 0.5f, 3.0f);
	glm::vec3 diffuseColor = glm::vec3(0.2f, 0.2f, 0.17f);

	// colors
	vec3f fogColor{ 0.6f, 0.6f, 0.8f };
	vec3f skyColor{ 0.15f, 0.15f, 0.9f };

#pragma pack(push)
#pragma pack(1)
	struct VertexData {
		GLfloat xyz[3];
		GLfloat rgba[4];
	};

	struct HudVertexData {
		GLfloat xy[2];
		GLfloat rgba[4];
	};
#pragma pack(pop)

public:
	GL3Renderer(Client *client, Graphics *graphics, SDL_Window *window);
	~GL3Renderer();

	void tick();

	void resize();
	void makePerspectiveMatrix();
	void makeOrthogonalMatrix();
	void makeMaxFOV();

	float getMaxFOV() { return maxFOV; }

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);

private:
	void loadShaderPrograms();
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);

	void buildCrossHair();
	void buildSky();

	void render();
	void renderSky();
	void renderMenu();
	void renderTarget();
	void renderPlayers();
	void renderHud(const Player &player);
};

#endif // GL_3_RENDERER_HPP
