#include "gl3_renderer.hpp"

#include <fstream>
#include <SDL2/SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

#include "graphics.hpp"
#include "menu.hpp"
#include "stopwatch.hpp"

#include "game/world.hpp"
#include "game/chunk.hpp"

using namespace gui;

GL3Renderer::GL3Renderer(
		Graphics *graphics,
		SDL_Window *window,
		World *world,
		const Menu *menu,
		const ClientState *state,
		const uint8 *localClientID,
		const GraphicsConf &conf,
		Stopwatch *stopwatch) :
		graphics(graphics),
		conf(conf),
		window(window),
		world(world),
		menu(menu),
		localClientID(*localClientID),
		stopwatch(stopwatch) {
	loadShaders();

	GLuint vertexArrayID;
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// Generate 1 buffer, put the resulting identifier in vertexBuffer
	glGenBuffers(1, &vertexBuffer);

	GLfloat vertexBufferData[] = {
	   -1.0f, -1.0f, -1.0f,
	   1.0f, -1.0f, -1.0f,
	   0.0f,  1.0f, -1.0f,
	};

	// The following commands will talk about our 'vertexBuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);
}

GL3Renderer::~GL3Renderer() {
	LOG(DEBUG, "Destroying Graphics");
	// TODO
}

void GL3Renderer::loadShaders() {
	// Create the shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string vertexShaderCode;
	std::ifstream vertexShaderStream("shaders/vertex_shader.vert", std::ios::in);
	if(vertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(vertexShaderStream, Line))
			vertexShaderCode += "\n" + Line;
		vertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderStream("shaders/fragment_shader.frag", std::ios::in);
	if(fragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(fragmentShaderStream, Line))
			fragmentShaderCode += "\n" + Line;
		fragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	LOG(DEBUG, "Compiling vertex shader");
	char const * VertexSourcePointer = vertexShaderCode.c_str();
	glShaderSource(vertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(vertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(vertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	LOG(DEBUG, "Compiling fragment shader");
	char const * FragmentSourcePointer = fragmentShaderCode.c_str();
	glShaderSource(fragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(fragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(fragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	LOG(DEBUG, "Linking program");
	progLoc = glCreateProgram();
	glAttachShader(progLoc, vertexShaderID);
	glAttachShader(progLoc, fragmentShaderID);
	glLinkProgram(progLoc);

	// Check the program
	glGetProgramiv(progLoc, GL_LINK_STATUS, &Result);
	glGetProgramiv(progLoc, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> programErrorMessage(std::max(InfoLogLength, int(1)));
	glGetProgramInfoLog(progLoc, InfoLogLength, NULL, &programErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &programErrorMessage[0]);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	glUseProgram(progLoc);

	// save uniform locations
	projMatLoc = glGetUniformLocation(progLoc, "projectionMatrix");
	viewMatLoc = glGetUniformLocation(progLoc, "viewMatrix");
	modelMatLoc = glGetUniformLocation(progLoc, "modelMatrix");
}

void GL3Renderer::resize(int width, int height) {
	LOG(INFO, "Resize to " << width << "x" << height);
	glViewport(0, 0, width, height);
	makePerspectiveMatrix();
	makeOrthogonalMatrix();
	// TODO
}

void GL3Renderer::makePerspectiveMatrix() {
	double normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	double currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	double angle;

	float yfov = conf.fov / normalRatio * TAU / 360.0;
	if (currentRatio > normalRatio)
		angle = atan(tan(yfov / 2) * normalRatio / currentRatio) * 2;
	else
		angle = yfov;

	float zFar = Chunk::WIDTH * sqrt(3 * (conf.render_distance + 1) * (conf.render_distance + 1));
	perspectiveMatrix = glm::perspective((float) angle,
			(float) currentRatio, ZNEAR, zFar);
}

void GL3Renderer::makeOrthogonalMatrix() {
	float normalRatio = DEFAULT_WINDOWED_RES[0] / (double) DEFAULT_WINDOWED_RES[1];
	float currentRatio = graphics->getWidth() / (double) graphics->getHeight();
	if (currentRatio > normalRatio)
		orthogonalMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[0] / 2.0f, DEFAULT_WINDOWED_RES[0] / 2.0f, -DEFAULT_WINDOWED_RES[0]
				/ currentRatio / 2.0f, DEFAULT_WINDOWED_RES[0] / currentRatio / 2.0f, 1.0f, -1.0f);
	else
		orthogonalMatrix = glm::ortho(-DEFAULT_WINDOWED_RES[1] * currentRatio / 2.0f, DEFAULT_WINDOWED_RES[1]
				* currentRatio / 2.0f, -DEFAULT_WINDOWED_RES[1] / 2.0f,
				DEFAULT_WINDOWED_RES[1] / 2.0f, 1.0f, -1.0f);
}

void GL3Renderer::setPerspectiveMatrix() {
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
}

void GL3Renderer::setOrthogonalMatrix() {
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(orthogonalMatrix));
}

void GL3Renderer::setViewMatrix() {
	glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
}

void GL3Renderer::setModelMatrix() {
	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
}

void GL3Renderer::makeMaxFOV() {
	float ratio = (float) DEFAULT_WINDOWED_RES[0] / DEFAULT_WINDOWED_RES[1];
	float yfov = conf.fov / ratio * TAU / 360.0;
	if (ratio < 1.0)
		maxFOV = yfov;
	else
		maxFOV = atan(ratio * tan(yfov / 2)) * 2;
}

void GL3Renderer::setDebug(bool debugActive) {
	// TODO
}

bool GL3Renderer::isDebug() {
	// TODO
	return false;
}

void GL3Renderer::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;
	// TODO
}

void GL3Renderer::tick() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	setPerspectiveMatrix();

	Player &player = world->getPlayer(localClientID);
	if (player.isValid()) {

		modelMatrix = glm::mat4(1.0f);
		setModelMatrix();

		// Render sky
		viewMatrix = glm::rotate(glm::mat4(1.0f), (float) (-player.getPitch() / 360.0 * TAU), glm::vec3(1.0f, 0.0f, 0.0f));
		setViewMatrix();
		//renderSky();

		// Render Scene
		viewMatrix = glm::rotate(viewMatrix, (float) (-player.getYaw() / 360.0 * TAU), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (-TAU / 4.0), glm::vec3(1.0f, 0.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, (float) (TAU / 4.0), glm::vec3(0.0f, 0.0f, 1.0f));
		vec3i64 playerPos = player.getPos();
		int64 m = RESOLUTION * Chunk::WIDTH;
		viewMatrix = glm::translate(viewMatrix, glm::vec3(
			(float) -((playerPos[0] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[1] % m + m) % m) / RESOLUTION,
			(float) -((playerPos[2] % m + m) % m) / RESOLUTION)
		);
		setViewMatrix();
	}

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(
	   0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	   3,                  // size
	   GL_FLOAT,           // type
	   GL_FALSE,           // normalized?
	   0,                  // stride
	   (void*)0            // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle

	glDisableVertexAttribArray(0);

	stopwatch->start(CLOCK_FLP);
	SDL_GL_SwapWindow(window);
	stopwatch->stop(CLOCK_FLP);
	// TODO
}
