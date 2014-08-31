#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <chrono>
#include "world.hpp"
#include "util.hpp"

class Graphics {
private:
	static const int START_WIDTH = 1600;
	static const int START_HEIGHT = 900;
	const double YFOV = TAU / 8;
	static const int VIEW_RANGE = 16;

	int width = START_WIDTH;
	int height = START_HEIGHT;

	SDL_GLContext glContext;
	SDL_Window *window;

	World *world;
	uint localClientID;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
	int lastFPS = 0;
	int64 lastFPSUpdate = 0;
	int frameCounter = 0;
	int lastNewQuads = 0;

	using DLMap = std::unordered_map<vec3i64, GLuint, size_t (*)(vec3i64 v)>;
	DLMap displayLists;

	float lightPosition[4] = {3.0, 2.0, 9.0};
	GLuint blockTexture;
	//TrueTypeFont font;

	GLdouble perspectiveMatrix[16];
	GLdouble orthogonalMatrix[16];

public:
	Graphics(World *world, int localClientID);
	~Graphics();

	void tick();

	void resize(int width, int height);
	void grab();

	bool isGrabbed();

	bool getCloseRequested();

private:
	void initGL();

	void makePerspective();
	void makeOrthogonal();

	void switchToPerspective();
	void switchToOrthogonal();

	double getMaxFOV();

	double getDrawWidth();
	double getDrawHeight();

	void render();
	void renderChunks();
	void renderChunk(Chunk &c, bool targeted, vec3ui8 ticc, int td);
	void renderPlayers();

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void removeDisplayLists();
};

#endif // GRAPHICS_HPP
