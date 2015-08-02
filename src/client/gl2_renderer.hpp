#ifndef GL2_RENDERER_HPP
#define GL2_RENDERER_HPP

#include <GL/glew.h>
#include <FTGL/ftgl.h>

#include "renderer.hpp"

#include "gl2_chunk_renderer.hpp"
#include "gl2_sky_renderer.hpp"
#include "gl2_hud_renderer.hpp"
#include "gl2_menu_renderer.hpp"
#include "gl2_debug_renderer.hpp"

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
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	Client *client;

	TextureManager texManager;

	GL2ChunkRenderer chunkRenderer;
	GL2SkyRenderer skyRenderer;
	GL2HudRenderer hudRenderer;
	GL2MenuRenderer menuRenderer;
	GL2DebugRenderer debugRenderer;

	double ZNEAR = 0.1f;

	static const int MAX_NEW_QUADS = 6000;
	static const int MAX_NEW_CHUNKS = 500;

	double maxFOV;

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

	vec3i64 oldPlayerChunk;

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
	GLuint fbo_depth_buffer = 0;

//	bool fxaa = false;

public:
	GL2Renderer(Client *client);
	~GL2Renderer();

	void tick();
	void resize();
	void setConf(const GraphicsConf &, const GraphicsConf &);

	TextureManager *getTextureManager();
	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

private:
	void initGL();

	void createFBO();
	void destroyFBO();

	void makePerspective();
	void makeOrthogonal();
	void makeFog();
	void makeMaxFOV();

	void switchToPerspective();
	void switchToOrthogonal();

	void render();
};

#endif // GL2_RENDERER_HPP
