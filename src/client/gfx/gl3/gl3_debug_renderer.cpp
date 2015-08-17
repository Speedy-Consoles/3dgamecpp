#include "gl3_debug_renderer.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "shared/engine/stopwatch.hpp"
#include "client/client.hpp"
#include "client/gfx/graphics.hpp"

#include "gl3_chunk_renderer.hpp"
#include "gl3_renderer.hpp"

static const vec3f relColors[] = {
	{0.6f, 0.6f, 1.0f},
	{0.0f, 0.0f, 0.8f},
	{0.6f, 0.0f, 0.8f},
	{0.0f, 0.6f, 0.8f},
	{0.4f, 0.4f, 0.8f},
	{0.7f, 0.7f, 0.0f},
	{0.8f, 0.8f, 0.3f},
	{0.0f, 0.8f, 0.0f},
	{0.0f, 0.4f, 0.0f},
	{0.7f, 0.1f, 0.7f},
	{0.7f, 0.7f, 0.4f},
	{0.2f, 0.6f, 0.6f},
	{0.8f, 0.0f, 0.0f},
};

GL3DebugRenderer::GL3DebugRenderer(Client *client, GL3Renderer *renderer, GL3ChunkRenderer *chunkRenderer) :
	client(client),
	renderer(renderer),
	chunkRenderer(chunkRenderer),
	font(&((GL3Renderer *) renderer)->getShaderManager()->getFontShader())
{
	GL(GenVertexArrays(1, &vao));
	GL(GenBuffers(1, &vbo));
	GL(BindVertexArray(vao));
	GL(BindBuffer(GL_ARRAY_BUFFER, vbo));
	GL(VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, (void *) 0));
	GL(VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (void *) 8));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
	GL(BindVertexArray(0));

	font.load("fonts/dejavusansmono20.fnt");
	font.setEncoding(Font::Encoding::UTF8);

	for (int i = 0; i < 20; i++) {
		prevFPS[i] = 0;
	}
}

GL3DebugRenderer::~GL3DebugRenderer() {
	GL(DeleteVertexArrays(1, &vao));
	GL(DeleteBuffers(1, &vbo));
}


void GL3DebugRenderer::tick() {
	if (getCurrentTime() - lastStopWatchSave > millis(200)) {
		lastStopWatchSave = getCurrentTime();
		client->getStopwatch()->stop(CLOCK_ALL);
		client->getStopwatch()->save();
		client->getStopwatch()->start(CLOCK_ALL);
	}

	PACKED(
	struct VertexData {
		GLfloat xy[2];
		GLfloat rgba[4];
	});

	VertexData vertexData[CLOCK_ID_NUM * 6];

	int vertexIndices[6] = {0, 1, 2, 2, 3, 0};

	float relStart = 0.0f;
	float relEnd = 0.0f;
	float center_positions[CLOCK_ID_NUM];
	numFaces = 0;

	int vIndex = 0;
	for (uint i = 0; i < CLOCK_ID_NUM; ++i) {
		relStart = relEnd;
		relEnd += client->getStopwatch()->getRel(i);
		vec2f vertexPositions[4] = {{0, relStart}, {1, relStart}, {1, relEnd}, {0, relEnd}};
		for (size_t j = 0; j < 6; j++) {
			vertexData[vIndex].rgba[0] = relColors[i][0];
			vertexData[vIndex].rgba[1] = relColors[i][1];
			vertexData[vIndex].rgba[2] = relColors[i][2];
			vertexData[vIndex].rgba[3] = 1.0f;
			vertexData[vIndex].xy[0] = vertexPositions[vertexIndices[j]][0];
			vertexData[vIndex].xy[1] = vertexPositions[vertexIndices[j]][1];
			vIndex++;
		}
		center_positions[i] = (relEnd + relStart) / 2.0f;
		numFaces += 2;
	}

	GL(BindBuffer(GL_ARRAY_BUFFER, vbo));
	GL(BufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));

	static const float REL_THRESHOLD = 0.001f;
	num_displayed_labels = 0;
	for (int i = 0; i < CLOCK_ID_NUM; ++i) {
		if (client->getStopwatch()->getRel(i) > REL_THRESHOLD)
			labeled_ids[num_displayed_labels++] = i;
	}

	used_positions[0] = 0.0;
	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		used_positions[i + 1] = center_positions[id];
	}
	used_positions[num_displayed_labels + 1] = 1.0;

	for (int iteration = 0; iteration < 3; ++iteration)
	for (int i = 1; i < num_displayed_labels + 1; ++i) {
		float d1 = used_positions[i] - used_positions[i - 1];
		float d2 = used_positions[i + 1] - used_positions[i];
		float diff = 2e-4f * (1.0f / d1 - 1.0f / d2);
		used_positions[i] += clamp(diff, -0.02f, 0.02f);
	}
}

void GL3DebugRenderer::render() {
	if (client->isDebugOn() && client->getState() == Client::State::PLAYING) {
		renderDebug();
		if (client->getStopwatch())
			renderPerformance();
	}
}

void GL3DebugRenderer::renderDebug() {
	while (getCurrentTime() - lastFPSUpdate > millis(50)) {
		lastFPSUpdate += millis(50);
		fpsSum -= prevFPS[fpsIndex];
		fpsSum += fpsCounter;
		prevFPS[fpsIndex] = fpsCounter;
		fpsCounter = 0;
		fpsIndex = (fpsIndex + 1) % 20;
	}
	fpsCounter++;

	font.setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	float x = (float) (-client->getGraphics()->getDrawWidth() / 2 + 5);
	float y = (float) (client->getGraphics()->getDrawHeight() / 2 - font.getTopOffset() - 5);

	char buffer[1024];
#define RENDER_LINE(...) sprintf(buffer, __VA_ARGS__);\
			font.write(x, y, 0.0f, buffer, 0);\
			y -= font.getLineHeight()

	RENDER_LINE("FPS: %d", fpsSum);

	const Player &player = client->getLocalPlayer();
	RENDER_LINE(" ");
	RENDER_LINE("PLAYER INFO:");
	RENDER_LINE("x: %" PRId64 " (%" PRId64 ")", player.getPos()[0],
		(int64)floor(player.getPos()[0] / (double)RESOLUTION));
	RENDER_LINE("y: %" PRId64 " (%" PRId64 ")", player.getPos()[1],
		(int64)floor(player.getPos()[1] / (double)RESOLUTION));
	RENDER_LINE("z: %" PRId64 " (%" PRId64 ")", player.getPos()[2],
		(int64)floor((player.getPos()[2] - Player::EYE_HEIGHT - 1) / (double)RESOLUTION));
	RENDER_LINE("yaw:   %6.1f", player.getYaw());
	RENDER_LINE("pitch: %6.1f", player.getPitch());
	RENDER_LINE("xvel: %8.1f", player.getVel()[0]);
	RENDER_LINE("yvel: %8.1f", player.getVel()[1]);
	RENDER_LINE("zvel: %8.1f", player.getVel()[2]);

	const World *world = client->getWorld();
	RENDER_LINE(" ");
	RENDER_LINE("WORLD INFO:");
	RENDER_LINE("needed chunks: %lu", (unsigned long) world->getNumNeededChunks());

	ChunkRendererDebugInfo crdi = chunkRenderer->getDebugInfo();
	RENDER_LINE(" ");
	RENDER_LINE("CHUNK RENDERER INFO:");
	RENDER_LINE("checked distance: %d", crdi.checkedDistance);
	RENDER_LINE("new faces: %d", crdi.newFaces);
	RENDER_LINE("new chunks: %d", crdi.newChunks);
	RENDER_LINE("total faces: %d", crdi.totalFaces);
	RENDER_LINE("visible chunks: %d", crdi.visibleChunks);
	RENDER_LINE("visible faces: %d", crdi.visibleFaces);
	RENDER_LINE("build queue size: %d", crdi.buildQueueSize);

	const ChunkManager *chunkManager = client->getChunkManager();
	RENDER_LINE(" ");
	RENDER_LINE("CHUNK MANAGER INFO:");
	RENDER_LINE("needed chunks: %d", chunkManager->getNumNeededChunks());
	RENDER_LINE("allocated chunks: %d", chunkManager->getNumAllocatedChunks());
	RENDER_LINE("loaded chunks: %d", chunkManager->getNumLoadedChunks());
	RENDER_LINE("requested queue size: %d", chunkManager->getRequestedQueueSize());
	RENDER_LINE("not-in-cache queue size: %d", chunkManager->getNotInCacheQueueSize());
}

void GL3DebugRenderer::renderPerformance() {
	HudShader *hudShader = &((GL3Renderer *) renderer)->getShaderManager()->getHudShader();
	float height = client->getGraphics()->getDrawHeight();
	float width = 20.0f;
	float moveRight = client->getGraphics()->getDrawWidth() / 2.0f - width;
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(moveRight, 0.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(width, height, 1.0f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.5f, 0.0f));
	hudShader->setModelMatrix(modelMatrix);
	hudShader->useProgram();
	GL(BindVertexArray(vao));
	GL(DrawArrays(GL_TRIANGLES, 0, numFaces * 3));

	static const char *relNames[CLOCK_ID_NUM] = {
		"WOT", // world tick
		"CRT", // chunk renderer tick
		"BCH", // build chunk
		"CRR", // chunk renderer render
		"IBQ", // iterate render queue
		"VS", // visibility search
		"FSH", // glFinish
		"FLP", // flip
		"SYN", // synchronize
		"ALL"
	};

	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		float x = client->getGraphics()->getDrawWidth() / 2.0f - 60;
		float y = (used_positions[i + 1] - 0.5f) * client->getGraphics()->getDrawHeight() - 14;

		char buffer[1024];
		sprintf(buffer, "%s", relNames[id]);
		auto color = relColors[id];
		font.setColor(glm::vec4(color[0], color[1], color[2], 1.0f));
		font.write(x, y, 0.0f, buffer, 0);
	}
}
