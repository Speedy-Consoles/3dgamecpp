#ifndef WITHOUT_GL2

#ifndef GL2_RENDERER_HPP
#define GL2_RENDERER_HPP

#include "client/gfx/renderer.hpp"

#include <GL/glew.h>
#include <FTGL/ftgl.h>

#include "shared/engine/vmath.hpp"
#include "shared/block_utils.hpp"
#include "shared/constants.hpp"
#include "shared/game/chunk.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gfx/component_renderer.hpp"
#include "client/client.hpp"
#include "client/config.hpp"

#include "gl2_texture_manager.hpp"

class World;
class Character;
class Menu;
class Stopwatch;

class GL2ChunkRenderer;
class GL2CharacterRenderer;
class GL2TargetRenderer;
class GL2SkyRenderer;
class GL2CrosshairRenderer;
class GL2HudRenderer;
class GL2MenuRenderer;
class GL2DebugRenderer;

class GL2Renderer : public Renderer {
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	Client *client;

	GL2TextureManager texManager;
	
	GL2ChunkRenderer *p_chunkRenderer;
	std::unique_ptr<ComponentRenderer> chunkRenderer;
	std::unique_ptr<ComponentRenderer> characterRenderer;
	std::unique_ptr<ComponentRenderer> targetRenderer;
	std::unique_ptr<ComponentRenderer> skyRenderer;
	std::unique_ptr<ComponentRenderer> crosshairRenderer;
	std::unique_ptr<ComponentRenderer> hudRenderer;
	std::unique_ptr<ComponentRenderer> menuRenderer;
	std::unique_ptr<ComponentRenderer> debugRenderer;

	float ZNEAR = 0.1f;

	float maxFOV;

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

	vec3i64 oldCharacterChunk;

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

	void tick() override;
	void resize() override;
	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	void rebuildChunk(vec3i64 chunkCoords) override;

	GL2TextureManager *getTextureManager();

	virtual float getMaxFOV() { return maxFOV; }

private:
	void initGL();

	void createFBO(AntiAliasing antiAliasing);
	void destroyFBO();

	void makePerspective(int renderDistance, float fieldOfView);
	void makeOrthogonal();
	void makeFog(int renderDistance);
	void makeMaxFOV(float fieldOfView);

	void switchToPerspective();
	void switchToOrthogonal();

	void render();
};

#endif // GL2_RENDERER_HPP
#endif // WITHOUT_GL2
