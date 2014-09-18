#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <chrono>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <FTGL/ftgl.h>

#include "world.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "stopwatch.hpp"
#include "client.hpp"
#include "config.hpp"
#include "menu.hpp"

class Graphics {
private:
	//static const int VIEW_RANGE = CHUNK_LOAD_RANGE;
	const double YFOV = TAU / 5;

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
	Menu *menu;
	uint localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
	int lastFPS = 0;
	int64 lastFPSUpdate = 0;
	int frameCounter = 0;
	int newQuads = 0;

	vec3i64 oldPlayerChunk;

	GLuint firstDL;
	vec3i64 *dlChunks;
	bool *dlHasChunk;

	float fogColor[3] = {0.6, 0.6, 0.8};
	float skyColor[3] = {0.15, 0.15, 0.9};
	float sunLightPosition[4] = {3.0, 2.0, 9.0, 0.0};
	GLuint blockTexture;
	GLuint noTexture;
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

	bool menuActive = true;
	bool debugActive = false;
	double oldRelMouseX = 0.5;
	double oldRelMouseY = 0.5;

	Stopwatch *stopwatch;

public:
	Graphics(World *world, Menu *menu, int localClientID, const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
	~Graphics();

	void tick();

	void resize(int width, int height);

	void setMenu(bool menuActive);
	bool isMenu();

	void setDebug(bool debugActive);
	bool isDebug();

	bool getCloseRequested();

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);

	int getHeight();
	int getWidth();

private:
	void initGL();

//	GLuint loadShader(const char *, GLenum);
//	GLuint loadProgram(const char *, const char *);
//
	void createFBO();
	void destroyFBO();

	void makePerspective();
	void makeOrthogonal();

	void makeFog();

	void switchToPerspective();
	void switchToOrthogonal();

	void calcDrawArea();

	void render();

	void renderMenu();

	void renderScene(const Player &);
	void renderHud(const Player &);
	void renderDebugInfo(const Player &);
	void renderPerformance();
	void renderChunks();
	void renderChunk(const Chunk &c, bool targeted, vec3ui8 ticc, int td);
	void renderPlayers();

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);
};

#endif // GRAPHICS_HPP
