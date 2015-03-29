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
    GLuint fontVertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint defaultFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint blockFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint hudFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint fontFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);

	LOG(DEBUG, "Building default vertex shader");
	buildShader(defaultVertexShaderLoc, "shaders/default_vertex_shader.vert");
	LOG(DEBUG, "Building block vertex shader");
    buildShader(blockVertexShaderLoc, "shaders/block_vertex_shader.vert");
    LOG(DEBUG, "Building hud vertex shader");
    buildShader(hudVertexShaderLoc, "shaders/hud_vertex_shader.vert");
    LOG(DEBUG, "Building font vertex shader");
    buildShader(fontVertexShaderLoc, "shaders/font.vert");

	LOG(DEBUG, "Building default fragment shader");
	buildShader(defaultFragmentShaderLoc, "shaders/default_fragment_shader.frag");
	LOG(DEBUG, "Building block fragment shader");
    buildShader(blockFragmentShaderLoc, "shaders/block_fragment_shader.frag");
    LOG(DEBUG, "Building hud fragment shader");
    buildShader(hudFragmentShaderLoc, "shaders/hud_fragment_shader.frag");
    LOG(DEBUG, "Building font fragment shader");
    buildShader(fontFragmentShaderLoc, "shaders/font.frag");

	// create the programs
	programLocations[DEFAULT_PROGRAM] = glCreateProgram();
    programLocations[BLOCK_PROGRAM] = glCreateProgram();
    programLocations[HUD_PROGRAM] = glCreateProgram();
    programLocations[FONT_PROGRAM] = glCreateProgram();

	GLuint defaultProgramShaderLocs[2] = {defaultVertexShaderLoc, defaultFragmentShaderLoc};
    GLuint blockProgramShaderLocs[2] = { blockVertexShaderLoc, blockFragmentShaderLoc };
    GLuint hudProgramShaderLocs[2] = { hudVertexShaderLoc, hudFragmentShaderLoc };
    GLuint fontProgramShaderLocs[2] = { fontVertexShaderLoc, fontFragmentShaderLoc };

	LOG(DEBUG, "Building default program");
	buildProgram(programLocations[DEFAULT_PROGRAM], defaultProgramShaderLocs, 2);
	LOG(DEBUG, "Building block program");
    buildProgram(programLocations[BLOCK_PROGRAM], blockProgramShaderLocs, 2);
    LOG(DEBUG, "Building hud program");
    buildProgram(programLocations[HUD_PROGRAM], hudProgramShaderLocs, 2);
    LOG(DEBUG, "Building font program");
    buildProgram(programLocations[FONT_PROGRAM], fontProgramShaderLocs, 2);

	// delete the shaders
	glDeleteShader(defaultVertexShaderLoc);
    glDeleteShader(blockVertexShaderLoc);
    glDeleteShader(hudVertexShaderLoc);
    glDeleteShader(fontVertexShaderLoc);
	glDeleteShader(defaultFragmentShaderLoc);
    glDeleteShader(blockFragmentShaderLoc);
    glDeleteShader(hudFragmentShaderLoc);
    glDeleteShader(fontFragmentShaderLoc);
	logOpenGLError();

	// get uniform locations
	defaultLightEnabledLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "lightEnabled");
	defaultAmbientColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "ambientLightColor");
	defaultDiffColorLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightColor");
	defaultDiffDirLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "diffuseLightDirection");
	defaultModelMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "modelMatrix");
	defaultViewMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "viewMatrix");
	defaultProjMatLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "projectionMatrix");
	defaultFogEnabledLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "fogEnabled");
	defaultFogStartDistanceLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "fogStartDistance");
	defaultFogEndDistanceLoc = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "fogEndDistance");

	blockLightEnabledLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "lightEnabled");
	blockAmbientColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "ambientLightColor");
	blockDiffColorLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightColor");
	blockDiffDirLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "diffuseLightDirection");
	blockModelMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "modelMatrix");
	blockViewMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "viewMatrix");
	blockProjMatLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "projectionMatrix");
	blockFogEnabledLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "fogEnabled");
	blockFogStartDistanceLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "fogStartDistance");
	blockFogEndDistanceLoc = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "fogEndDistance");

    hudProjMatLoc = glGetUniformLocation(programLocations[HUD_PROGRAM], "projectionMatrix");

    fontTransMatLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "transformMatrix");
	fontIsPackedLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "isPacked");
	fontHasOutlineLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "hasOutline");
	fontPageLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "page");
	fontChannelLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "chnl");
	fontTextColorLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "textColor");
	fontOutlineColorLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "outlineColor");
	fontModeLoc = glGetUniformLocation(programLocations[FONT_PROGRAM], "mode");

    logOpenGLError();

	glUseProgram(programLocations[DEFAULT_PROGRAM]);
	GLint tmp = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "textureSampler");
	glUniform1i(tmp, 0);
	tmp = glGetUniformLocation(programLocations[DEFAULT_PROGRAM], "fogSampler");
	glUniform1i(tmp, 1);

    logOpenGLError();

	glUseProgram(programLocations[BLOCK_PROGRAM]);
	tmp = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "textureSampler");
	glUniform1i(tmp, 0);
	tmp = glGetUniformLocation(programLocations[BLOCK_PROGRAM], "fogSampler");
	glUniform1i(tmp, 1);

    logOpenGLError();

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
    glDeleteProgram(programLocations[FONT_PROGRAM]);
}

void Shaders::buildShader(GLuint shaderLoc, const char* fileName) {
	// Read the Vertex Shader code from the file
	std::string shaderCode;
	std::ifstream shaderStream(fileName, std::ios::in);
	if (shaderStream.is_open()) {
		std::string Line = "";
		while (getline(shaderStream, Line))
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
	if (!Result)
		LOG(ERROR, &shaderErrorMessage[0]);
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
	if (!Result)
		LOG(ERROR, &programErrorMessage[0]);
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
	blockModelMatUp = false;
}

void Shaders::setViewMatrix(const glm::mat4 &matrix) {
	viewMatrix = matrix;
	defaultViewMatUp = false;
	blockViewMatUp = false;
}

void Shaders::setProjectionMatrix(const glm::mat4 &matrix) {
	projectionMatrix = matrix;
	defaultProjMatUp = false;
	blockProjMatUp = false;
}

void Shaders::setHudProjectionMatrix(const glm::mat4 &matrix) {
	hudProjectionMatrix = matrix;
	hudProjMatUp = false;
}

void Shaders::setFontProjectionMatrix(const glm::mat4 &matrix) {
    fontProjectionMatrix = matrix;
    fontProjMatUp = false;
}

void Shaders::setFontModelMatrix(const glm::mat4 &matrix) {
    fontModelMatrix = matrix;
    fontModelMatUp = false;
}

void Shaders::setFontIsPacked(bool isPacked) {
    fontIsPacked = isPacked;
    fontIsPackedUp = false;
}

void Shaders::setFontHasOutline(bool hasOutline) {
	fontHasOutline = hasOutline;
	fontHasOutlineUp = false;
}

void Shaders::setFontPage(short page) {
    fontPage = page;
    fontPageUp = false;
}

void Shaders::setFontChannel(short channel) {
    fontChannel = channel;
    fontChannelUp = false;
}

void Shaders::setFontTextColor(const glm::vec4 &color) {
	fontTextColor = color;
	fontTextColorUp = false;
}

void Shaders::setFontOutlineColor(const glm::vec4 &color) {
	fontOutlineColor = color;
	fontOutlineColorUp = false;
}

void Shaders::setFontMode(FontRenderMode mode) {
	fontMode = static_cast<int>(mode);
	fontModeUp = false;
}

void Shaders::setFogEnabled(bool enabled) {
	fogEnabled = enabled;
	defaultFogEnabledUp = false;
	blockFogEnabledUp = false;
}

void Shaders::setStartFogDistance(float distance) {
	fogStartDistance = distance;
	defaultFogStartDistanceUp = false;
	blockFogStartDistanceUp = false;
}

void Shaders::setEndFogDistance(float distance) {
	fogEndDistance = distance;
	defaultFogEndDistanceUp = false;
	blockFogEndDistanceUp = false;
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
		if (!defaultFogEnabledUp) {
			glUniform1i(defaultFogEnabledLoc, fogEnabled);
			defaultFogEnabledUp = true;
		}
		if (!defaultFogStartDistanceUp) {
			glUniform1f(defaultFogStartDistanceLoc, fogStartDistance);
			defaultFogStartDistanceUp = true;
		}
		if (!defaultFogEndDistanceUp) {
			glUniform1f(defaultFogEndDistanceLoc, fogEndDistance);
			defaultFogEndDistanceUp = true;
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
		if (!blockFogEnabledUp) {
			glUniform1i(blockFogEnabledLoc, fogEnabled);
			blockFogEnabledUp = true;
		}
		if (!blockFogStartDistanceUp) {
			glUniform1f(blockFogStartDistanceLoc, fogStartDistance);
			blockFogStartDistanceUp = true;
		}
		if (!blockFogEndDistanceUp) {
			glUniform1f(blockFogEndDistanceLoc, fogEndDistance);
			blockFogEndDistanceUp = true;
		}
		break;
    case FONT_PROGRAM:
		if (!fontProjMatUp || !fontModelMatUp) {
			auto transformMat = fontProjectionMatrix * fontModelMatrix;
			glUniformMatrix4fv(fontTransMatLoc, 1, GL_FALSE, glm::value_ptr(transformMat));
            fontProjMatUp = true;
			fontModelMatUp = true;
        }
		if (!fontIsPackedUp) {
			glUniform1i(fontIsPackedLoc, fontIsPacked);
			fontIsPackedUp = true;
		}
		if (!fontHasOutlineUp) {
			glUniform1i(fontHasOutlineLoc, fontHasOutline);
			fontHasOutlineUp = true;
		}
        if (!fontPageUp) {
            glUniform1i(fontPageLoc, fontPage);
            fontPageUp = true;
		}
		if (!fontChannelUp) {
			glUniform1i(fontChannelLoc, fontChannel);
			fontChannelUp = true;
		}
		if (!fontTextColorUp) {
			glUniform4fv(fontTextColorLoc, 1, glm::value_ptr(fontTextColor));
			fontTextColorUp = true;
		}
		if (!fontOutlineColorUp) {
			glUniform4fv(fontOutlineColorLoc, 1, glm::value_ptr(fontOutlineColor));
			fontOutlineColorUp = true;
		}
		if (!fontModeUp) {
			glUniform1i(fontModeLoc, fontMode);
			fontModeUp = true;
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
