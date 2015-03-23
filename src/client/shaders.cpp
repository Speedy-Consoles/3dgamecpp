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
	GLuint skyFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint blockFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint hudFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);

	LOG(DEBUG, "Building default vertex shader");
	buildShader(defaultVertexShaderLoc, "shaders/default_vertex_shader.vert");
	LOG(DEBUG, "Building block vertex shader");
	buildShader(blockVertexShaderLoc, "shaders/block_vertex_shader.vert");
	LOG(DEBUG, "Building block vertex shader");
	buildShader(hudVertexShaderLoc, "shaders/hud_vertex_shader.vert");
	LOG(DEBUG, "Building default fragment shader");
	buildShader(defaultFragmentShaderLoc, "shaders/default_fragment_shader.frag");
	LOG(DEBUG, "Building sky fragment shader");
	buildShader(skyFragmentShaderLoc, "shaders/sky_fragment_shader.frag");
	LOG(DEBUG, "Building block fragment shader");
	buildShader(blockFragmentShaderLoc, "shaders/block_fragment_shader.frag");
	LOG(DEBUG, "Building hud fragment shader");
	buildShader(hudFragmentShaderLoc, "shaders/hud_fragment_shader.frag");

	// create the programs
	programLocations[DEFAULT_PROGRAM] = glCreateProgram();
	programLocations[SKY_PROGRAM] = glCreateProgram();
	programLocations[BLOCK_PROGRAM] = glCreateProgram();
	programLocations[HUD_PROGRAM] = glCreateProgram();

	GLuint defaultProgramShaderLocs[2] = {defaultVertexShaderLoc, defaultFragmentShaderLoc};
	GLuint skyProgramShaderLocs[2] = {defaultVertexShaderLoc, skyFragmentShaderLoc};
	GLuint blockProgramShaderLocs[2] = {blockVertexShaderLoc, blockFragmentShaderLoc};
	GLuint hudProgramShaderLocs[2] = {hudVertexShaderLoc, hudFragmentShaderLoc};

	LOG(DEBUG, "Building default program");
	buildProgram(programLocations[DEFAULT_PROGRAM], defaultProgramShaderLocs, 2);
	LOG(DEBUG, "Building sky program");
	buildProgram(programLocations[SKY_PROGRAM], skyProgramShaderLocs, 2);
	LOG(DEBUG, "Building block program");
	buildProgram(programLocations[BLOCK_PROGRAM], blockProgramShaderLocs, 2);
	LOG(DEBUG, "Building hud program");
	buildProgram(programLocations[HUD_PROGRAM], hudProgramShaderLocs, 2);

	// delete the shaders
	glDeleteShader(defaultVertexShaderLoc);
	glDeleteShader(blockVertexShaderLoc);
	glDeleteShader(hudVertexShaderLoc);
	glDeleteShader(defaultFragmentShaderLoc);
	glDeleteShader(skyFragmentShaderLoc);
	glDeleteShader(blockFragmentShaderLoc);
	glDeleteShader(hudFragmentShaderLoc);
	logOpenGLError();

	// get uniform locations
	defaultLightEnabledLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "lightEnabled");
	defaultAmbientColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "ambientLightColor");
	defaultDiffColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightColor");
	defaultDiffDirLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightDirection");
	defaultModelMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "modelMatrix");
	defaultViewMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "viewMatrix");
	defaultProjMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "projectionMatrix");
	defaultFogDistanceLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "fogDistance");

	skyModelMatLoc = glGetUniformLocation(programLocations[SKY_PROGRAM], "modelMatrix");
	skyViewMatLoc = glGetUniformLocation(programLocations[SKY_PROGRAM], "viewMatrix");
	skyProjMatLoc = glGetUniformLocation(programLocations[SKY_PROGRAM], "projectionMatrix");

	blockLightEnabledLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "lightEnabled");
	blockAmbientColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "ambientLightColor");
	blockDiffColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightColor");
	blockDiffDirLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightDirection");
	blockModelMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "modelMatrix");
	blockViewMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "viewMatrix");
	blockProjMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "projectionMatrix");
	blockFogDistanceLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "fogDistance");

	hudProjMatLoc = glGetUniformLocation(programLocations[HUD_PROGRAM], "projectionMatrix");


//	GLint tmp = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "sampler");
//	glUseProgram(programLocations[DEFAULT_PROGRAM]);
//	glUniform1i(tmp, 0);

	glUseProgram(programLocations[SKY_PROGRAM]);
	GLint tmp = glGetUniformLocation(programLocations[SKY_PROGRAM], "sceneColorSampler");
	glUniform1i(tmp, 0);
	tmp = glGetUniformLocation(programLocations[SKY_PROGRAM], "sceneDistanceSampler");
	glUniform1i(tmp, 1);
	tmp = glGetUniformLocation(programLocations[SKY_PROGRAM], "sceneDepthSampler");
	glUniform1i(tmp, 2);

	glUseProgram(programLocations[BLOCK_PROGRAM]);
	tmp = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "sampler");
	glUniform1i(tmp, 0);

//	glUseProgram(programLocations[HUD_PROGRAM]);
//	tmp = glGetUniformLocation(programLocations[HUD_PROGRAM], "sampler");
//	glUniform1i(tmp, 0);


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
	if(shaderStream.is_open()) {
		std::string Line = "";
		while(getline(shaderStream, Line))
			shaderCode += "\n" + Line;
		shaderStream.close();
	} else {
		LOG(FATAL, "Could not open file!");
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

void Shaders::setLightEnabled(bool enabled) {
	lightEnabled = enabled;
	defaultLightEnabledUp = false;
	blockLightEnabledUp = false;
}

void Shaders::setDiffuseLightColor(const glm::vec3 &color) {
	diffuseColor = color;
	defaultDiffColorUp = false;
	blockDiffColorUp = false;
}

void Shaders::setDiffuseLightDirection(const glm::vec3 &direction) {
	diffuseDirection = direction;
	defaultDiffDirUp = false;
	blockDiffDirUp = false;
}

void Shaders::setAmbientLightColor(const glm::vec3 &color) {
	ambientColor = color;
	defaultAmbientColorUp = false;
	blockAmbientColorUp = false;
}

void Shaders::setModelMatrix(const glm::mat4 &matrix) {
	modelMatrix = matrix;
	defaultModelMatUp = false;
	skyModelMatUp = false;
	blockModelMatUp = false;
}

void Shaders::setViewMatrix(const glm::mat4 &matrix) {
	viewMatrix = matrix;
	defaultViewMatUp = false;
	skyViewMatUp = false;
	blockViewMatUp = false;
}

void Shaders::setProjectionMatrix(const glm::mat4 &matrix) {
	projectionMatrix = matrix;
	defaultProjMatUp = false;
	skyProjMatUp = false;
	blockProjMatUp = false;
}

void Shaders::setHudProjectionMatrix(const glm::mat4 &matrix) {
	hudProjectionMatrix = matrix;
	hudProjMatUp = false;
}

void Shaders::setFogDistance(float distance) {
	fogDistance = distance;
	defaultFogDistanceUp = false;
	blockFogDistanceUp = false;
}

void Shaders::prepareProgram(ShaderProgram program) {
	if (activeProgram != program) {
		glUseProgram(programLocations[program]);
		activeProgram = program;
	}
	switch (program) {
	case DEFAULT_PROGRAM:
		if (!defaultLightEnabledUp) {
			glUniform1i(defaultLightEnabledLoc, lightEnabled);
			defaultLightEnabledUp = true;
		}
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
		if (!defaultFogDistanceUp) {
			glUniform1f(defaultFogDistanceLoc, fogDistance);
			defaultFogDistanceUp = true;
		}
		break;
	case SKY_PROGRAM:
		if (!skyModelMatUp) {
			glUniformMatrix4fv(skyModelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			skyModelMatUp = true;
		}
		if (!skyViewMatUp) {
			glUniformMatrix4fv(skyViewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			skyViewMatUp = true;
		}
		if (!skyProjMatUp) {
			glUniformMatrix4fv(skyProjMatLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
			skyProjMatUp = true;
		}
		break;
	case BLOCK_PROGRAM:
		if (!blockLightEnabledUp) {
			glUniform1i(blockLightEnabledLoc, lightEnabled);
			blockLightEnabledUp = true;
		}
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
		if (!blockFogDistanceUp) {
			glUniform1f(blockFogDistanceLoc, fogDistance);
			blockFogDistanceUp = true;
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
