#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <FTGL/ftgl.h>

#include "vmath.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "client.hpp"
#include "config.hpp"
#include "texture_manager.hpp"

class World;
class Player;
class Chunk;
class Menu;
class Stopwatch;

namespace gui {
	class Frame;
	class Label;
	class Widget;
	class Button;
}

class Graphics {
private:
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	double ZNEAR = 0.1f;

	static const int MAX_NEW_QUADS = 3000;
	static const int MAX_NEW_CHUNKS = 50;

	int width;
	int height;

	double drawWidth;
	double drawHeight;

	double maxFOV;

	SDL_GLContext glContext;
	SDL_Window *window;

	GraphicsConf conf;
	World *world;
	const Menu *menu;
	const ClientState &state;
	ClientState oldState;
	const uint8 &localClientID;

	int prevFPS[20];
	int fpsCounter = 0;
	int fpsSum = 0;
	size_t fpsIndex = 0;
	time_t lastFPSUpdate = 0;
	time_t lastStopWatchSave = 0;
	int newFaces = 0;
	int newChunks = 0;
	int faces = 0;
	int visibleChunks = 0;
	int visibleFaces = 0;

	vec3i64 oldPlayerChunk;

	GLuint firstDL;
	vec3i64 *dlChunks;
	uint8 *dlStatus;
	int *dlFaces;

	uint16 *passThroughs;
	uint8 *exits;
	bool *visited;
	vec3i64 *fringe;
	int *indices;
	int fringeCapacity;

	vec3f fogColor{ 0.6f, 0.6f, 0.8f };
	vec3f skyColor{ 0.15f, 0.15f, 0.9f };
	vec4f sunLightPosition{ 3.0f, 2.0f, 9.0f, 0.0f };

	TextureManager texManager;
	FTFont *font;

//	GLuint program = 0;
//	GLuint program_postproc = 0;

	GLdouble perspectiveMatrix[16];
	GLdouble orthogonalMatrix[16];

	GLuint fbo = 0;
	GLuint fbo_color_buffer = 0;
//	GLuint fbo_texture = 0;
	GLuint fbo_depth_buffer = 0;

//	bool fxaa = false;

	bool debugActive = false;
	double oldRelMouseX = 0.5;
	double oldRelMouseY = 0.5;

	Stopwatch *stopwatch;

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

private:
	void initGL();

//	GLuint loadShader(const char *, GLenum);
//	GLuint loadProgram(const char *, const char *);

	void setMenu(bool menuActive);

	void createFBO();
	void destroyFBO();

	void makePerspective();
	void makeOrthogonal();

	void makeFog();
	void makeMaxFOV();

	void switchToPerspective();
	void switchToOrthogonal();

	void calcDrawArea();

	void render();

	void renderMenu();

	void renderScene();
	void renderSky();
	void renderChunks();
	void renderChunk(Chunk &c);
	void renderPlayers();
	void renderTarget();

	void renderHud(const Player &);
	void renderDebugInfo(const Player &);
	void renderPerformance();

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void renderWidget(const gui::Widget *);
	void renderFrame(const gui::Frame *);
	void renderLabel(const gui::Label *);
	void renderButton(const gui::Button *);
	void renderText(const char *text);
};

#endif // GRAPHICS_HPP
