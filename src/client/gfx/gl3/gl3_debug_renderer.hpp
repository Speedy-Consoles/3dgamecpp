#ifndef GL3_DEBUG_RENDERER_HPP_
#define GL3_DEBUG_RENDERER_HPP_

#include "client/client.hpp"
#include "client/gfx/component_renderer.hpp"

#include "gl3_font.hpp"

class GL3Renderer;
class GL3ChunkRenderer;

class GL3DebugRenderer : public ComponentRenderer {
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;
	GL3ChunkRenderer *chunkRenderer = nullptr;

	BMFont font;

	int numFaces = 0;
	int num_displayed_labels;
	uint labeled_ids[CLOCK_ID_NUM];
	float used_positions[CLOCK_ID_NUM + 2];

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

	void tick() override;
	void render() override;

private:
	void renderDebug();
	void renderPerformance();
};

#endif //GL3_DEBUG_RENDERER_HPP_
