#include "gl3_debug_renderer.hpp"

#include "gl3_chunk_renderer.hpp"
#include "gl3_renderer.hpp"

GL3DebugRenderer::GL3DebugRenderer(Client *client, GL3Renderer *renderer, GL3ChunkRenderer *chunkRenderer, ShaderManager *shaderManager) :
	client(client),
	renderer(renderer),
	chunkRenderer(chunkRenderer),
	font(&shaderManager->getFontShader())
{
	font.load("fonts/dejavusansmono20.fnt");
	font.setEncoding(Font::Encoding::UTF8);
}

void GL3DebugRenderer::render() {
	if (getCurrentTime() - lastStopWatchSave > millis(200)) {
		lastStopWatchSave = getCurrentTime();
		client->getStopwatch()->stop(CLOCK_ALL);
		client->getStopwatch()->save();
		client->getStopwatch()->start(CLOCK_ALL);
	}

	while (getCurrentTime() - lastFPSUpdate > millis(50)) {
		lastFPSUpdate += millis(50);
		fpsSum -= prevFPS[fpsIndex];
		fpsSum += fpsCounter;
		prevFPS[fpsIndex] = fpsCounter;
		fpsCounter = 0;
		fpsIndex = (fpsIndex + 1) % 20;
	}
	fpsCounter++;

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
	RENDER_LINE("render queue size: %d", crdi.renderQueueSize);

	const ChunkManager *chunkManager = client->getChunkManager();
	RENDER_LINE(" ");
	RENDER_LINE("CHUNK MANAGER INFO:");
	RENDER_LINE("needed chunks: %d", chunkManager->getNumNeededChunks());
	RENDER_LINE("allocated chunks: %d", chunkManager->getNumAllocatedChunks());
	RENDER_LINE("loaded chunks: %d", chunkManager->getNumLoadedChunks());
	RENDER_LINE("requested queue size: %d", chunkManager->getRequestedQueueSize());
	RENDER_LINE("not-in-cache queue size: %d", chunkManager->getNotInCacheQueueSize());
}
