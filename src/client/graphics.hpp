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
	double ZNEAR = 0.1f;

	static const int MAX_NEW_QUADS = 3000;

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
	int faces = 0;

	vec3i64 oldPlayerChunk;

	GLuint firstDL;
	vec3i64 *dlChunks;
	bool *dlHasChunk;
	int *dlFaces;

	float fogColor[3] = {0.6, 0.6, 0.8};
	float skyColor[3] = {0.15, 0.15, 0.9};
	float sunLightPosition[4] = {3.0, 2.0, 9.0, 0.0};

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
	int renderChunk(const Chunk &c);
	void renderPlayers();

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
