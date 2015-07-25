#ifndef GL_2_RENDERER_HPP
#define GL_2_RENDERER_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <FTGL/ftgl.h>

#include "renderer.hpp"

#include "graphics.hpp"
#include "engine/vmath.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "client.hpp"
#include "config.hpp"
#include "texture_manager.hpp"

#include "game/chunk.hpp"

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

class GL2Renderer : public Renderer {
private:
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	double ZNEAR = 0.1f;

	static const int MAX_NEW_QUADS = 6000;
	static const int MAX_NEW_CHUNKS = 500;

	double maxFOV;

	Client *client;
	Graphics *graphics;
	SDL_Window *window;

	// rendering helpers
	TextureManager texManager;
	FTFont *font;

	// performance info
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

	// cache
	int renderDistance;

	// display lists
	GLuint dlFirstAddress;
	vec3i64 *dlChunks;
	uint8 *dlStatus;

	// chunk data
	int *chunkFaces;
	uint16 *chunkPassThroughs;

	// visibility search for rendering
	uint8 *vsExits;
	bool *vsVisited;
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	vec3i64 oldPlayerChunk;

	// face buffer for chunk rendering
	int faceBufferIndices[255][(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3];
	float faceBuffer[(Chunk::WIDTH + 1) * Chunk::WIDTH * Chunk::WIDTH * 3 * (3 + 4 * (2 + 3 + 3))];

	// transformation matrices
	GLdouble perspectiveMatrix[16];
	GLdouble orthogonalMatrix[16];

	// colors
	vec3f fogColor{ 0.6f, 0.6f, 0.8f };
	vec3f skyColor{ 0.15f, 0.15f, 0.9f };
	vec4f sunLightPosition{ 3.0f, 2.0f, 9.0f, 0.0f };


	// frame buffers for anti-aliasing
	GLuint fbo = 0;
	GLuint fbo_color_buffer = 0;
//	GLuint fbo_texture = 0;
	GLuint fbo_depth_buffer = 0;

//	bool fxaa = false;

public:
	GL2Renderer(Client *client, Graphics *graphics, SDL_Window *window);
	~GL2Renderer();

	void tick();
	void resize();
	void setConf(const GraphicsConf &, const GraphicsConf &);

private:
	void initGL();
	void initRenderDistanceDependent();
	void destroyRenderDistanceDependent();

	void createFBO();
	void destroyFBO();

	void makePerspective();
	void makeOrthogonal();

	void makeFog();
	void makeMaxFOV();

	void switchToPerspective();
	void switchToOrthogonal();

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

#endif // GL_2_RENDERER_HPP
