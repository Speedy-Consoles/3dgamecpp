#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <chrono>

#include <SDL2/SDL.h>
#include <FTGL/ftgl.h>

#include "world.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "stopwatch.hpp"
#include "client.hpp"
#include <GL/glew.h>

class Graphics {
private:
	static const int START_WIDTH = 1600;
	static const int START_HEIGHT = 900;
	static const int VIEW_RANGE = CHUNK_UNLOAD_RANGE;
	const double YFOV = TAU / 5;
	const double ZNEAR = 0.1f;
	const double ZFAR = Chunk::WIDTH * VIEW_RANGE - 2.5;
	static const int MAX_NEW_QUADS = 3000;

	int width;
	int height;

	double drawWidth;
	double drawHeight;

	double maxFOV;

	SDL_GLContext glContext;
	SDL_Window *window;

	World *world;
	uint localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
	int lastFPS = 0;
	int64 lastFPSUpdate = 0;
	int frameCounter = 0;
	int newQuads = 0;

	vec3i64 oldPlayerChunk;

	GLuint firstDL;
	vec3i64 *dlChunks;

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
	uint msaa = 0;
//
//	bool fxaa = false;

	Stopwatch *stopwatch;

public:
	Graphics(World *world, int localClientID, Stopwatch *stopwatch = nullptr);
	~Graphics();

	void tick();

	void resize(int width, int height);
	void grab();

	bool isGrabbed();

	bool getCloseRequested();

	void enableMSAA(uint samples = 4);
	void disableMSAA();
	uint getMSAA() const { return msaa; }
//	void enableFXAA();
//	void disableFXAA();
//	bool getFXAA() const { return fxaa; }

private:
	void initGL();

//	GLuint loadShader(const char *, GLenum);
//	GLuint loadProgram(const char *, const char *);
//
	void createFBO();
	void destroyFBO();

	void makePerspective();
	void makeOrthogonal();

	void switchToPerspective();
	void switchToOrthogonal();

	void calcDrawArea();

	void render();
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
