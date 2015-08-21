#ifndef GL3_RENDERER_HPP
#define GL3_RENDERER_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "shared/game/chunk.hpp"
#include "shared/block_utils.hpp"
#include "client/client.hpp"
#include "client/gfx/renderer.hpp"
#include "client/gfx/component_renderer.hpp"

#include "gl3_shaders.hpp"
#include "gl3_texture_manager.hpp"

class Graphics;
class World;
class Player;
class Menu;
class Stopwatch;

class GL3ChunkRenderer;
class GL3TargetRenderer;
class GL3SkyRenderer;
class GL3CrosshairRenderer;
class GL3HudRenderer;
class GL3MenuRenderer;
class GL3DebugRenderer;

class GL3Renderer : public Renderer {
private:
	Client *client = nullptr;

	// resources
	GL3ShaderManager shaderManager;
	GL3TextureManager texManager;

	// chunk renderer
	GL3ChunkRenderer *p_chunkRenderer;
	std::unique_ptr<ComponentRenderer> chunkRenderer;
	std::unique_ptr<ComponentRenderer> targetRenderer;
	std::unique_ptr<ComponentRenderer> skyRenderer;
	std::unique_ptr<ComponentRenderer> crosshairRenderer;
	std::unique_ptr<ComponentRenderer> hudRenderer;
	std::unique_ptr<ComponentRenderer> menuRenderer;
	std::unique_ptr<ComponentRenderer> debugRenderer;

	// 3D values
	float ZNEAR = 0.1f;
	float maxFOV;

	// fbos
	GLuint skyFbo = 0;
	GLuint skyTex = 0;

	// light
	glm::vec3 ambientColor = glm::vec3(0.4f, 0.4f, 0.35f);
	glm::vec3 diffuseDirection = glm::vec3(1.0f, 0.5f, 3.0f);
	glm::vec3 diffuseColor = glm::vec3(0.2f, 0.2f, 0.17f);

	// colors
	vec3f fogColor{ 0.6f, 0.6f, 0.8f };
	vec3f skyColor{ 0.15f, 0.15f, 0.9f };

	// frame buffers for anti-aliasing
	GLuint fbo = 0;
	GLuint fbo_color_buffer = 0;
	GLuint fbo_depth_buffer = 0;

public:
	GL3Renderer(Client *client);
	~GL3Renderer();

	void tick() override;
	void render() override;
	void resize() override;
	void setConf(const GraphicsConf &, const GraphicsConf &) override;

	void rebuildChunk(vec3i64 chunkCoords) override;

	GL3ShaderManager *getShaderManager() { return &shaderManager; }
	GL3TextureManager *getTextureManager() { return &texManager; }
	float getMaxFOV() override { return maxFOV; };


private:
	void createFBO(AntiAliasing antiAliasing);
	void destroyFBO();

	void makePerspectiveMatrix(int renderDistance, float fieldOfView);
	void makeOrthogonalMatrix();
	void makeSkyFbo();
	void makeMaxFOV(float fieldOfView);
};

#endif // GL_3_RENDERER_HPP
