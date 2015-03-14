#ifndef GL3_RENDERER_HPP
#define GL3_RENDERER_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "renderer.hpp"
#include "shaders.hpp"

#include "game/chunk.hpp"
#include "util.hpp"
#include "client.hpp"
#include "config.hpp"

class Graphics;
struct SDL_Window;
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

class GL3Renderer : public Renderer {
private:
	enum DisplayListStatus {
		NO_CHUNK = 0,
		OUTDATED,
		OK,
	};

	Graphics *graphics;
	GraphicsConf conf;

	SDL_Window *window;
	World *world;
	const Menu *menu;
	const ClientState &state;
	const uint8 &localClientID;
	Stopwatch *stopwatch;

	// 3D values
	float ZNEAR = 0.1f;
	float maxFOV;

	// performance limits
	static const int MAX_NEW_QUADS = 6000;
	static const int MAX_NEW_CHUNKS = 500;

	// shaders
	Shaders shaders;

	// vao, vbo locations
	GLuint *vaos;
	GLuint *vbos;
	GLuint crossHairVAO;
	GLuint crossHairVBO;

	// vao meta data
	vec3i64 *vaoChunks;
	uint8 *vaoStatus;

	// chunk data
	int *chunkFaces;
	uint16 *chunkPassThroughs;

	// visibility search for rendering
	uint8 *vsExits;
	bool *vsVisited;
	int vsFringeCapacity;
	vec3i64 *vsFringe;
	int *vsIndices;

	// performance info
	bool debugActive = false;
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

#pragma pack(push)
#pragma pack(1)
	// buffer for block vertices
	struct BlockVertexData {
		GLushort positionIndex;
		GLubyte dirIndexCornerIndex;
		GLubyte shadowLevels;
	};

	struct HudVertexData {
		GLfloat x;
		GLfloat y;
		GLfloat r;
		GLfloat g;
		GLfloat b;
		GLfloat a;
	};
#pragma pack(pop)

	BlockVertexData blockVertexBuffer[Chunk::WIDTH * Chunk::WIDTH * (Chunk::WIDTH + 1) * 3 * 2 * 3];

public:
	GL3Renderer(Graphics *graphics, SDL_Window *window, World *world, const Menu *menu,
				const ClientState *state, const uint8 *localClientId,
				const GraphicsConf &conf, Stopwatch *stopwatch = nullptr);
	~GL3Renderer();

	void tick();

	void resize();
	void makePerspectiveMatrix();
	void makeOrthogonalMatrix();
	void makeMaxFOV();

	void setDebug(bool debugActive);
	bool isDebug();

	bool getCloseRequested();

	const GraphicsConf &getConf() const { return conf; }
	void setConf(const GraphicsConf &);

private:
	void loadShaderPrograms();
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);

	void buildChunk(Chunk &c);
	void buildCrossHair();

	void render();
	void renderChunks();
	void renderMenu();
	void renderTarget();
	void renderPlayers();
	void renderHud(const Player &player);

	bool inFrustum(vec3i64 cc, vec3i64 pos, vec3d lookDir);

	void destroyRenderDistanceDependent();
	void initRenderDistanceDependent();
};

#endif // GL_3_RENDERER_HPP
