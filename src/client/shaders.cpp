#include "shaders.hpp"

#include <fstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

Shaders::Shaders() {
	// Create the shaders
	GLuint defaultVertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint blockVertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint hudVertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint defaultFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint blockFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);

	LOG(DEBUG, "Building default vertex shader");
	buildShader(defaultVertexShaderLoc, "shaders/default_vertex_shader.vert");
	LOG(DEBUG, "Building block vertex shader");
	buildShader(blockVertexShaderLoc, "shaders/block_vertex_shader.vert");
	LOG(DEBUG, "Building block vertex shader");
	buildShader(hudVertexShaderLoc, "shaders/hud_vertex_shader.vert");
	LOG(DEBUG, "Building default fragment shader");
	buildShader(defaultFragmentShaderLoc, "shaders/default_fragment_shader.frag");
	LOG(DEBUG, "Building block fragment shader");
	buildShader(blockFragmentShaderLoc, "shaders/block_fragment_shader.frag");

	// create the programs
	programLocations[DEFAULT_PROGRAM] = glCreateProgram();
	programLocations[BLOCK_PROGRAM] = glCreateProgram();
	programLocations[HUD_PROGRAM] = glCreateProgram();

	GLuint defaultProgramShaderLocs[2] = {defaultVertexShaderLoc, defaultFragmentShaderLoc};
	GLuint blockProgramShaderLocs[2] = {blockVertexShaderLoc, blockFragmentShaderLoc};
	GLuint hudProgramShaderLocs[2] = {hudVertexShaderLoc, defaultFragmentShaderLoc};

	LOG(DEBUG, "Building default program");
	buildProgram(programLocations[DEFAULT_PROGRAM], defaultProgramShaderLocs, 2);
	LOG(DEBUG, "Building block program");
	buildProgram(programLocations[BLOCK_PROGRAM], blockProgramShaderLocs, 2);
	LOG(DEBUG, "Building hud program");
	buildProgram(programLocations[HUD_PROGRAM], hudProgramShaderLocs, 2);

	// delete the shaders
	glDeleteShader(defaultVertexShaderLoc);
	glDeleteShader(blockVertexShaderLoc);
	glDeleteShader(hudVertexShaderLoc);
	glDeleteShader(defaultFragmentShaderLoc);
	glDeleteShader(blockFragmentShaderLoc);
	logOpenGLError();

	// get uniform locations
	blockAmbientColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "ambientLightColor");
	blockDiffColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightColor");
	blockDiffDirLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightDirection");
	blockModelMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "modelMatrix");
	blockViewMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "viewMatrix");
	blockProjMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "projectionMatrix");

	defaultAmbientColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "ambientLightColor");
	defaultDiffColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightColor");
	defaultDiffDirLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightDirection");
	defaultModelMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "modelMatrix");
	defaultViewMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "viewMatrix");
	defaultProjMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "projectionMatrix");

	hudProjMatLoc = glGetUniformLocation(programLocations[HUD_PROGRAM], "projectionMatrix");

	activeProgram = DEFAULT_PROGRAM;
	glUseProgram(programLocations[DEFAULT_PROGRAM]);
	logOpenGLError();
}

Shaders::~Shaders() {
	glDeleteProgram(programLocations[DEFAULT_PROGRAM]);
	glDeleteProgram(programLocations[BLOCK_PROGRAM]);
	glDeleteProgram(programLocations[HUD_PROGRAM]);
}

void Shaders::buildShader(GLuint shaderLoc, const char* fileName) {
	// Read the Vertex Shader code from the file
	std::string shaderCode;
	std::ifstream shaderStream(fileName, std::ios::in);
	if(shaderStream.is_open())
	{
		std::string Line = "";
		while(getline(shaderStream, Line))
			shaderCode += "\n" + Line;
		shaderStream.close();
	}

	// Compile Shader
	char const * sourcePointer = shaderCode.c_str();
	glShaderSource(shaderLoc, 1, &sourcePointer , NULL);
	glCompileShader(shaderLoc);

	// Check Vertex Shader
	GLint Result = GL_FALSE;
	int InfoLogLength;
	glGetShaderiv(shaderLoc, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(shaderLoc, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> shaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(shaderLoc, InfoLogLength, NULL, &shaderErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &shaderErrorMessage[0]);
}

void Shaders::buildProgram(GLuint programLoc, GLuint *shaders, int numShaders) {
	// Link the program
	for (int i = 0; i < numShaders; i++) {
		glAttachShader(programLoc, shaders[i]);
	}
	glLinkProgram(programLoc);

	// Check the program
	GLint Result = GL_FALSE;
	int InfoLogLength;
	glGetProgramiv(programLoc, GL_LINK_STATUS, &Result);
	glGetProgramiv(programLoc, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> programErrorMessage(std::max(InfoLogLength, int(1)));
	glGetProgramInfoLog(programLoc, InfoLogLength, NULL, &programErrorMessage[0]);
	// TODO use logging
	fprintf(stdout, "%s\n", &programErrorMessage[0]);
}

void Shaders::setDiffuseLightColor(const glm::vec3 &color) {
	diffuseColor = color;
	blockDiffColorUp = false;
	defaultDiffColorUp = false;
}

void Shaders::setDiffuseLightDirection(const glm::vec3 &direction) {
	diffuseDirection = direction;
	blockDiffDirUp = false;
	defaultDiffDirUp = false;
}

void Shaders::setAmbientLightColor(const glm::vec3 &color) {
	ambientColor = color;
	blockAmbientColorUp = false;
	defaultAmbientColorUp = false;
}

void Shaders::setModelMatrix(const glm::mat4 &matrix) {
	modelMatrix = matrix;
	blockModelMatUp = false;
	defaultModelMatUp = false;
}

void Shaders::setViewMatrix(const glm::mat4 &matrix) {
	viewMatrix = matrix;
	blockViewMatUp = false;
	defaultViewMatUp = false;
}

void Shaders::setProjectionMatrix(const glm::mat4 &matrix) {
	projectionMatrix = matrix;
	blockProjMatUp = false;
	defaultProjMatUp = false;
}

void Shaders::setHudProjectionMatrix(const glm::mat4 &matrix) {
	hudProjectionMatrix = matrix;
	hudProjMatUp = false;
}

void Shaders::prepareProgram(ShaderProgram program) {
	if (activeProgram != program) {
		glUseProgram(programLocations[program]);
		activeProgram = program;
	}
	switch (program) {
	case DEFAULT_PROGRAM:
		if (!defaultAmbientColorUp) {
			glUniform3fv(defaultAmbientColorLoc, 1, glm::value_ptr(ambientColor));
			defaultAmbientColorUp = true;
		}
		if (!defaultDiffColorUp) {
			glUniform3fv(defaultDiffColorLoc, 1, glm::value_ptr(diffuseColor));
			defaultDiffColorUp = true;
		}
		if (!defaultDiffDirUp) {
			glUniform3fv(defaultDiffDirLoc, 1, glm::value_ptr(diffuseDirection));
			defaultDiffDirUp = true;
		}
		if (!defaultModelMatUp) {
			glUniformMatrix4fv(defaultModelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			defaultModelMatUp = true;
		}
		if (!defaultViewMatUp) {
			glUniformMatrix4fv(defaultViewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			defaultViewMatUp = true;
		}
		if (!defaultProjMatUp) {
			glUniformMatrix4fv(defaultProjMatLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
			defaultProjMatUp = true;
		}
		break;
	case BLOCK_PROGRAM:
		if (!blockAmbientColorUp) {
			glUniform3fv(blockAmbientColorLoc, 1, glm::value_ptr(ambientColor));
			blockAmbientColorUp = true;
		}
		if (!blockDiffColorUp) {
			glUniform3fv(blockDiffColorLoc, 1, glm::value_ptr(diffuseColor));
			blockDiffColorUp = true;
		}
		if (!blockDiffDirUp) {
			glUniform3fv(blockDiffDirLoc, 1, glm::value_ptr(diffuseDirection));
			blockDiffDirUp = true;
		}
		if (!blockModelMatUp) {
			glUniformMatrix4fv(blockModelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			blockModelMatUp = true;
		}
		if (!blockViewMatUp) {
			glUniformMatrix4fv(blockViewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			blockViewMatUp = true;
		}
		if (!blockProjMatUp) {
			glUniformMatrix4fv(blockProjMatLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
			blockProjMatUp = true;
		}
		break;
	case HUD_PROGRAM:
		if (!hudProjMatUp) {
			glUniformMatrix4fv(hudProjMatLoc, 1, GL_FALSE, glm::value_ptr(hudProjectionMatrix));
			hudProjMatUp = true;
		}
		break;
	}
}
