#include "shaders.hpp"

#include <fstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

Shader::Shader(ShaderManager *manager, const char *vert_src, const char *frag_src) :
	manager(manager)
{
	GLuint defaultVertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint defaultFragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	LOG(TRACE, "Building '" << vert_src << "'");
	buildShader(defaultVertexShaderLoc, vert_src);
	LOG(TRACE, "Building '" << frag_src << "'");
	buildShader(defaultFragmentShaderLoc, frag_src);

	programLocation = glCreateProgram();
	GLuint defaultProgramShaderLocs[2] = {defaultVertexShaderLoc, defaultFragmentShaderLoc};

	LOG(TRACE, "Building program");
	buildProgram(programLocation, defaultProgramShaderLocs, 2);

	glDeleteShader(defaultVertexShaderLoc);
	glDeleteShader(defaultFragmentShaderLoc);
	
	logOpenGLError();
}

Shader::~Shader() {
	glDeleteProgram(programLocation);
}

GLuint Shader::getUniformLocation(const char *name) const {
	return glGetUniformLocation(programLocation, name);
}

void Shader::useProgram() {
	manager->useProgram(programLocation);
}

void Shader::buildShader(GLuint shaderLoc, const char* fileName) {
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

void Shader::buildProgram(GLuint programLoc, GLuint *shaders, int numShaders) {
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

DefaultShader::DefaultShader(ShaderManager *manager) :
	Shader(manager, "shaders/default.vert", "shaders/default.frag")
{
	defaultLightEnabledLoc = getUniformLocation("lightEnabled");
	defaultAmbientColorLoc = getUniformLocation("ambientLightColor");
	defaultDiffColorLoc = getUniformLocation("diffuseLightColor");
	defaultDiffDirLoc = getUniformLocation("diffuseLightDirection");
	defaultModelMatLoc = getUniformLocation("modelMatrix");
	defaultViewMatLoc = getUniformLocation("viewMatrix");
	defaultProjMatLoc = getUniformLocation("projectionMatrix");
	defaultFogEnabledLoc = getUniformLocation("fogEnabled");
	defaultFogStartDistanceLoc = getUniformLocation("fogStartDistance");
	defaultFogEndDistanceLoc = getUniformLocation("fogEndDistance");

	GLint tmp;
	glUseProgram(getProgramLocation());
	tmp = getUniformLocation("textureSampler");
	glUniform1i(tmp, 0);
	tmp = getUniformLocation("fogSampler");
	glUniform1i(tmp, 1);
}

void DefaultShader::useProgram() {
	Shader::useProgram();

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
}

void DefaultShader::setLightEnabled(bool enabled) {
	lightEnabled = enabled;
	defaultLightEnabledUp = false;
}

void DefaultShader::setDiffuseLightColor(const glm::vec3 &color) {
	diffuseColor = color;
	defaultDiffColorUp = false;
}

void DefaultShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	diffuseDirection = direction;
	defaultDiffDirUp = false;
}

void DefaultShader::setAmbientLightColor(const glm::vec3 &color) {
	ambientColor = color;
	defaultAmbientColorUp = false;
}

void DefaultShader::setModelMatrix(const glm::mat4 &matrix) {
	modelMatrix = matrix;
	defaultModelMatUp = false;
}

void DefaultShader::setViewMatrix(const glm::mat4 &matrix) {
	viewMatrix = matrix;
	defaultViewMatUp = false;
}

void DefaultShader::setProjectionMatrix(const glm::mat4 &matrix) {
	projectionMatrix = matrix;
	defaultProjMatUp = false;
}

void DefaultShader::setFogEnabled(bool enabled) {
	fogEnabled = enabled;
	defaultFogEnabledUp = false;
}

void DefaultShader::setStartFogDistance(float distance) {
	fogStartDistance = distance;
	defaultFogStartDistanceUp = false;
}

void DefaultShader::setEndFogDistance(float distance) {
	fogEndDistance = distance;
	defaultFogEndDistanceUp = false;
}

BlockShader::BlockShader(ShaderManager *manager) :
	Shader(manager, "shaders/block.vert", "shaders/block.frag")
{
	blockLightEnabledLoc = getUniformLocation("lightEnabled");
	blockAmbientColorLoc = getUniformLocation("ambientLightColor");
	blockDiffColorLoc = getUniformLocation("diffuseLightColor");
	blockDiffDirLoc = getUniformLocation("diffuseLightDirection");
	blockModelMatLoc = getUniformLocation("modelMatrix");
	blockViewMatLoc = getUniformLocation("viewMatrix");
	blockProjMatLoc = getUniformLocation("projectionMatrix");
	blockFogEnabledLoc = getUniformLocation("fogEnabled");
	blockFogStartDistanceLoc = getUniformLocation("fogStartDistance");
	blockFogEndDistanceLoc = getUniformLocation("fogEndDistance");

	GLint tmp;
	glUseProgram(getProgramLocation());
	tmp = getUniformLocation("textureSampler");
	glUniform1i(tmp, 0);
	tmp = getUniformLocation("fogSampler");
	glUniform1i(tmp, 1);
}

void BlockShader::useProgram() {
	Shader::useProgram();

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
}

void BlockShader::setLightEnabled(bool enabled) {
	lightEnabled = enabled;
	blockLightEnabledUp = false;
}

void BlockShader::setDiffuseLightColor(const glm::vec3 &color) {
	diffuseColor = color;
	blockDiffColorUp = false;
}

void BlockShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	diffuseDirection = direction;
	blockDiffDirUp = false;
}

void BlockShader::setAmbientLightColor(const glm::vec3 &color) {
	ambientColor = color;
	blockAmbientColorUp = false;
}

void BlockShader::setModelMatrix(const glm::mat4 &matrix) {
	modelMatrix = matrix;
	blockModelMatUp = false;
}

void BlockShader::setViewMatrix(const glm::mat4 &matrix) {
	viewMatrix = matrix;
	blockViewMatUp = false;
}

void BlockShader::setProjectionMatrix(const glm::mat4 &matrix) {
	projectionMatrix = matrix;
	blockProjMatUp = false;
}

void BlockShader::setFogEnabled(bool enabled) {
	fogEnabled = enabled;
	blockFogEnabledUp = false;
}

void BlockShader::setStartFogDistance(float distance) {
	fogStartDistance = distance;
	blockFogStartDistanceUp = false;
}

void BlockShader::setEndFogDistance(float distance) {
	fogEndDistance = distance;
	blockFogEndDistanceUp = false;
}

HudShader::HudShader(ShaderManager *manager) :
	Shader(manager, "shaders/hud.vert", "shaders/hud.frag")
{
	hudProjMatLoc = getUniformLocation("projectionMatrix");
}

void HudShader::useProgram() {
	Shader::useProgram();

	if (!hudProjMatUp) {
		glUniformMatrix4fv(hudProjMatLoc, 1, GL_FALSE, glm::value_ptr(hudProjectionMatrix));
		hudProjMatUp = true;
	}
}

void HudShader::setHudProjectionMatrix(const glm::mat4 &matrix) {
	hudProjectionMatrix = matrix;
	hudProjMatUp = false;
}

FontShader::FontShader(ShaderManager *manager) :
	Shader(manager, "shaders/font.vert", "shaders/font.frag")
{
	fontTransMatLoc = getUniformLocation("transformMatrix");
	fontIsPackedLoc = getUniformLocation("isPacked");
	fontHasOutlineLoc = getUniformLocation("hasOutline");
	fontPageLoc = getUniformLocation("page");
	fontChannelLoc = getUniformLocation("chnl");
	fontTextColorLoc = getUniformLocation("textColor");
	fontOutlineColorLoc = getUniformLocation("outlineColor");
	fontModeLoc = getUniformLocation("mode");
}

void FontShader::useProgram() {
	Shader::useProgram();

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
}

void FontShader::setFontProjectionMatrix(const glm::mat4 &matrix) {
    fontProjectionMatrix = matrix;
    fontProjMatUp = false;
}

void FontShader::setFontModelMatrix(const glm::mat4 &matrix) {
    fontModelMatrix = matrix;
    fontModelMatUp = false;
}

void FontShader::setFontIsPacked(bool isPacked) {
    fontIsPacked = isPacked;
    fontIsPackedUp = false;
}

void FontShader::setFontHasOutline(bool hasOutline) {
	fontHasOutline = hasOutline;
	fontHasOutlineUp = false;
}

void FontShader::setFontPage(short page) {
    fontPage = page;
    fontPageUp = false;
}

void FontShader::setFontChannel(short channel) {
    fontChannel = channel;
    fontChannelUp = false;
}

void FontShader::setFontTextColor(const glm::vec4 &color) {
	fontTextColor = color;
	fontTextColorUp = false;
}

void FontShader::setFontOutlineColor(const glm::vec4 &color) {
	fontOutlineColor = color;
	fontOutlineColorUp = false;
}

void FontShader::setFontMode(FontRenderMode mode) {
	fontMode = static_cast<int>(mode);
	fontModeUp = false;
}

ShaderManager::ShaderManager()
{
	programs.resize(NUM_PROGRAMS);
	programs[DEFAULT_PROGRAM] = std::make_unique<DefaultShader>(this);
	programs[BLOCK_PROGRAM] = std::make_unique<BlockShader>(this);
	programs[HUD_PROGRAM] = std::make_unique<HudShader>(this);
	programs[FONT_PROGRAM] = std::make_unique<FontShader>(this);
}

DefaultShader &ShaderManager::getDefaultShader() {
	return static_cast<DefaultShader &>(*programs[DEFAULT_PROGRAM]);
}

BlockShader &ShaderManager::getBlockShader() {
	return static_cast<BlockShader &>(*programs[BLOCK_PROGRAM]);
}

HudShader &ShaderManager::getHudShader() {
	return static_cast<HudShader &>(*programs[HUD_PROGRAM]);
}

FontShader &ShaderManager::getFontShader() {
	return static_cast<FontShader &>(*programs[FONT_PROGRAM]);
}

void ShaderManager::useProgram(GLuint program) {
	if (activeProgram != program) {
		glUseProgram(program);
		activeProgram = program;
	}
}
