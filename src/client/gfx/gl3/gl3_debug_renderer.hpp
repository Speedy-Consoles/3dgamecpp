#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "shared/game/world.hpp"
#include "client/gfx/graphics.hpp"

#include "gl3_font.hpp"

class GL3Renderer;
class GL3ChunkRenderer;

class GL3DebugRenderer {
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;
	GL3ChunkRenderer *chunkRenderer = nullptr;

	BMFont font;

	int numFaces = 0;
	GLuint vao = 0;
	GLuint vbo = 0;

	int prevFPS[20];
	int fpsCounter = 0;
	int fpsSum = 0;
	size_t fpsIndex = 0;
	time_t lastFPSUpdate = 0;
	time_t lastStopWatchSave = 0;
public:
	GL3DebugRenderer(Client *client, GL3Renderer *renderer, GL3ChunkRenderer *chunkRenderer);
	~GL3DebugRenderer();

	void tick();
	void render();

private:
	void renderDebug();
	void renderPerformance();
};

#endif //GL3_DEBUG_RENDERER_HPP_
