#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <chrono>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <FTGL/ftgl.h>

#include "world.hpp"
#include "util.hpp"
#include "constants.hpp"

class Graphics {
private:
	static const int START_WIDTH = 1600;
	static const int START_HEIGHT = 900;
	const double YFOV = TAU / 8;
	static const int VIEW_RANGE = CHUNK_UNLOAD_RANGE;
	static const int MAX_NEW_QUADS = 5000;

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

	using DLMap = std::unordered_map<vec3i64, GLuint, size_t (*)(vec3i64 v)>;
	DLMap displayLists;

	float sunLightPosition[4] = {3.0, 2.0, 9.0, 0.0};
	GLuint blockTexture;
	FTFont *font;

	GLdouble perspectiveMatrix[16];
	GLdouble orthogonalMatrix[16];

	enum DurationType {
		DUR_CLR, // clearing
		DUR_NDL, // new display lists
		DUR_DLC, // display list calls
		DUR_CHL, // chunk lookup
		DUR_OCH, // other chunk
		DUR_PLA, // player rendering
		DUR_HUD, // hud rendering
		DUR_FLP, // flipping
		DUR_TIC, // world ticking
		DUR_UAF, // unaccounted for
	};
	static const uint DUR_TYPE_NUM = 9;
	float rel_durs[DUR_TYPE_NUM + 1];
	std::chrono::microseconds durs[9];
	float last_frame_microseconds;
	float last_frame_start_point;

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

	void calcDrawArea();

	void render();
	void renderChunks();
	void renderChunk(const Chunk &c, bool targeted, vec3ui8 ticc, int td);
	void renderPlayers();

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void removeDisplayLists();
};

#endif // GRAPHICS_HPP
