#include "shaders.hpp"

#include <fstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "engine/logging.hpp"
#include "engine/math.hpp"

Shader::Shader(ShaderManager *manager, const char *vert_src, const char *frag_src) :
	_manager(manager)
{
	GLuint VertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	LOG(TRACE, "Building '" << vert_src << "'");
	buildShader(VertexShaderLoc, vert_src);
	LOG(TRACE, "Building '" << frag_src << "'");
	buildShader(FragmentShaderLoc, frag_src);

	_programLocation = glCreateProgram();
	GLuint ProgramShaderLocs[2] = {VertexShaderLoc, FragmentShaderLoc};

	LOG(TRACE, "Building program");
	buildProgram(_programLocation, ProgramShaderLocs, 2);

	glDeleteShader(VertexShaderLoc);
	glDeleteShader(FragmentShaderLoc);
	
	logOpenGLError();
}

Shader::~Shader() {
	glDeleteProgram(_programLocation);
}

GLuint Shader::getUniformLocation(const char *name) const {
	return glGetUniformLocation(_programLocation, name);
}

void Shader::useProgram() {
	_manager->useProgram(_programLocation);
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
	_lightEnabledLoc = getUniformLocation("lightEnabled");
	_ambientColorLoc = getUniformLocation("ambientLightColor");
	_diffuseColorLoc = getUniformLocation("diffuseLightColor");
	_diffuseDirectionLoc = getUniformLocation("diffuseLightDirection");
	_modelMatrixLoc = getUniformLocation("modelMatrix");
	_viewMatrixLoc = getUniformLocation("viewMatrix");
	_projectionMatrixLoc = getUniformLocation("projectionMatrix");
	_fogEnabledLoc = getUniformLocation("fogEnabled");
	_fogStartDistanceLoc = getUniformLocation("fogStartDistance");
	_fogEndDistanceLoc = getUniformLocation("fogEndDistance");

	GLint tmp;
	glUseProgram(getProgramLocation());
	tmp = getUniformLocation("textureSampler");
	glUniform1i(tmp, 0);
	tmp = getUniformLocation("fogSampler");
	glUniform1i(tmp, 1);
}

void DefaultShader::useProgram() {
	Shader::useProgram();

	if (!_lightEnabledDirty) {
		glUniform1i(_lightEnabledLoc, _lightEnabled);
		_lightEnabledDirty = true;
	}
	if (!_ambientColorDirty) {
		glUniform3fv(_ambientColorLoc, 1, glm::value_ptr(_ambientColor));
		_ambientColorDirty = true;
	}
	if (!_diffuseColorDirty) {
		glUniform3fv(_diffuseColorLoc, 1, glm::value_ptr(_diffuseColor));
		_diffuseColorDirty = true;
	}
	if (!_diffuseDirectionDirty) {
		glUniform3fv(_diffuseDirectionLoc, 1, glm::value_ptr(_diffuseDirection));
		_diffuseDirectionDirty = true;
	}
	if (!_modelMatrixDirty) {
		glUniformMatrix4fv(_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(_modelMatrix));
		_modelMatrixDirty = true;
	}
	if (!_viewMatrixDirty) {
		glUniformMatrix4fv(_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(_viewMatrix));
		_viewMatrixDirty = true;
	}
	if (!_projectionMatrixDirty) {
		glUniformMatrix4fv(_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
		_projectionMatrixDirty = true;
	}
	if (!_fogEnabledDirty) {
		glUniform1i(_fogEnabledLoc, _fogEnabled);
		_fogEnabledDirty = true;
	}
	if (!_fogStartDistanceDirty) {
		glUniform1f(_fogStartDistanceLoc, _fogStartDistance);
		_fogStartDistanceDirty = true;
	}
	if (!_fogEndDistanceDirty) {
		glUniform1f(_fogEndDistanceLoc, _fogEndDistance);
		_fogEndDistanceDirty = true;
	}
}

void DefaultShader::setLightEnabled(bool enabled) {
	_lightEnabled = enabled;
	_lightEnabledDirty = false;
}

void DefaultShader::setDiffuseLightColor(const glm::vec3 &color) {
	_diffuseColor = color;
	_diffuseColorDirty = false;
}

void DefaultShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	_diffuseDirection = direction;
	_diffuseDirectionDirty = false;
}

void DefaultShader::setAmbientLightColor(const glm::vec3 &color) {
	_ambientColor = color;
	_ambientColorDirty = false;
}

void DefaultShader::setModelMatrix(const glm::mat4 &matrix) {
	_modelMatrix = matrix;
	_modelMatrixDirty = false;
}

void DefaultShader::setViewMatrix(const glm::mat4 &matrix) {
	_viewMatrix = matrix;
	_viewMatrixDirty = false;
}

void DefaultShader::setProjectionMatrix(const glm::mat4 &matrix) {
	_projectionMatrix = matrix;
	_projectionMatrixDirty = false;
}

void DefaultShader::setFogEnabled(bool enabled) {
	_fogEnabled = enabled;
	_fogEnabledDirty = false;
}

void DefaultShader::setStartFogDistance(float distance) {
	_fogStartDistance = distance;
	_fogStartDistanceDirty = false;
}

void DefaultShader::setEndFogDistance(float distance) {
	_fogEndDistance = distance;
	_fogEndDistanceDirty = false;
}

BlockShader::BlockShader(ShaderManager *manager) :
	Shader(manager, "shaders/block.vert", "shaders/block.frag")
{
	_lightEnabledLoc = getUniformLocation("lightEnabled");
	_ambientColorLoc = getUniformLocation("ambientLightColor");
	_diffuseColorLoc = getUniformLocation("diffuseLightColor");
	_diffuseDirectionLoc = getUniformLocation("diffuseLightDirection");
	_modelMatrixLoc = getUniformLocation("modelMatrix");
	_viewMatrixLoc = getUniformLocation("viewMatrix");
	_projectionMatrixLoc = getUniformLocation("projectionMatrix");
	_fogEnabledLoc = getUniformLocation("fogEnabled");
	_fogStartDistanceLoc = getUniformLocation("fogStartDistance");
	_fogEndDistanceLoc = getUniformLocation("fogEndDistance");

	GLint tmp;
	glUseProgram(getProgramLocation());
	tmp = getUniformLocation("textureSampler");
	glUniform1i(tmp, 0);
	tmp = getUniformLocation("fogSampler");
	glUniform1i(tmp, 1);
}

void BlockShader::useProgram() {
	Shader::useProgram();

	if (!_lightEnabledDirty) {
		glUniform1i(_lightEnabledLoc, _lightEnabled);
		_lightEnabledDirty = true;
	}
	if (!_ambientColorDirty) {
		glUniform3fv(_ambientColorLoc, 1, glm::value_ptr(_ambientColor));
		_ambientColorDirty = true;
	}
	if (!_diffuseColorDirty) {
		glUniform3fv(_diffuseColorLoc, 1, glm::value_ptr(_diffuseColor));
		_diffuseColorDirty = true;
	}
	if (!_diffuseDirectionDirty) {
		glUniform3fv(_diffuseDirectionLoc, 1, glm::value_ptr(_diffuseDirection));
		_diffuseDirectionDirty = true;
	}
	if (!_modelMatrixDirty) {
		glUniformMatrix4fv(_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(_modelMatrix));
		_modelMatrixDirty = true;
	}
	if (!_viewMatrixDirty) {
		glUniformMatrix4fv(_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(_viewMatrix));
		_viewMatrixDirty = true;
	}
	if (!_projectionMatrixDirty) {
		glUniformMatrix4fv(_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
		_projectionMatrixDirty = true;
	}
	if (!_fogEnabledDirty) {
		glUniform1i(_fogEnabledLoc, _fogEnabled);
		_fogEnabledDirty = true;
	}
	if (!_fogStartDistanceDirty) {
		glUniform1f(_fogStartDistanceLoc, _fogStartDistance);
		_fogStartDistanceDirty = true;
	}
	if (!_fogEndDistanceDirty) {
		glUniform1f(_fogEndDistanceLoc, _fogEndDistance);
		_fogEndDistanceDirty = true;
	}
}

void BlockShader::setLightEnabled(bool enabled) {
	_lightEnabled = enabled;
	_lightEnabledDirty = false;
}

void BlockShader::setDiffuseLightColor(const glm::vec3 &color) {
	_diffuseColor = color;
	_diffuseColorDirty = false;
}

void BlockShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	_diffuseDirection = direction;
	_diffuseDirectionDirty = false;
}

void BlockShader::setAmbientLightColor(const glm::vec3 &color) {
	_ambientColor = color;
	_ambientColorDirty = false;
}

void BlockShader::setModelMatrix(const glm::mat4 &matrix) {
	_modelMatrix = matrix;
	_modelMatrixDirty = false;
}

void BlockShader::setViewMatrix(const glm::mat4 &matrix) {
	_viewMatrix = matrix;
	_viewMatrixDirty = false;
}

void BlockShader::setProjectionMatrix(const glm::mat4 &matrix) {
	_projectionMatrix = matrix;
	_projectionMatrixDirty = false;
}

void BlockShader::setFogEnabled(bool enabled) {
	_fogEnabled = enabled;
	_fogEnabledDirty = false;
}

void BlockShader::setStartFogDistance(float distance) {
	_fogStartDistance = distance;
	_fogStartDistanceDirty = false;
}

void BlockShader::setEndFogDistance(float distance) {
	_fogEndDistance = distance;
	_fogEndDistanceDirty = false;
}

HudShader::HudShader(ShaderManager *manager) :
	Shader(manager, "shaders/hud.vert", "shaders/hud.frag")
{
	_projectionMatrixLoc = getUniformLocation("projectionMatrix");
}

void HudShader::useProgram() {
	Shader::useProgram();

	if (!_projectionMatrixDirty) {
		glUniformMatrix4fv(_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
		_projectionMatrixDirty = true;
	}
}

void HudShader::setProjectionMatrix(const glm::mat4 &matrix) {
	_projectionMatrix = matrix;
	_projectionMatrixDirty = false;
}

FontShader::FontShader(ShaderManager *manager) :
	Shader(manager, "shaders/font.vert", "shaders/font.frag")
{
	_mvpMatrixLoc = getUniformLocation("mvpMatrix");
	_isPackedLoc = getUniformLocation("isPacked");
	_hasOutlineLoc = getUniformLocation("hasOutline");
	_pageLoc = getUniformLocation("page");
	_channelLoc = getUniformLocation("chnl");
	_textColorLoc = getUniformLocation("textColor");
	_outlineColorLoc = getUniformLocation("outlineColor");
	_modeLoc = getUniformLocation("mode");
}

void FontShader::useProgram() {
	Shader::useProgram();

	if (!_projectionMatrixDirty || !_modelMatrixDirty) {
		auto mvpMatrix = _projectionMatrix * _modelMatrix;
		glUniformMatrix4fv(_mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        _projectionMatrixDirty = true;
		_modelMatrixDirty = true;
    }
	if (!_isPackedDirty) {
		glUniform1i(_isPackedLoc, _isPacked);
		_isPackedDirty = true;
	}
	if (!_hasOutlineDirty) {
		glUniform1i(_hasOutlineLoc, _hasOutline);
		_hasOutlineDirty = true;
	}
    if (!_pageDirty) {
        glUniform1i(_pageLoc, _page);
        _pageDirty = true;
	}
	if (!_channelDirty) {
		glUniform1i(_channelLoc, _channel);
		_channelDirty = true;
	}
	if (!_textColorDirty) {
		glUniform4fv(_textColorLoc, 1, glm::value_ptr(_textColor));
		_textColorDirty = true;
	}
	if (!_outlineColorDirty) {
		glUniform4fv(_outlineColorLoc, 1, glm::value_ptr(_outlineColor));
		_outlineColorDirty = true;
	}
	if (!_modeDirty) {
		glUniform1i(_modeLoc, _mode);
		_modeDirty = true;
	}
}

void FontShader::setProjectionMatrix(const glm::mat4 &matrix) {
    _projectionMatrix = matrix;
    _projectionMatrixDirty = false;
}

void FontShader::setModelMatrix(const glm::mat4 &matrix) {
    _modelMatrix = matrix;
    _modelMatrixDirty = false;
}

void FontShader::setIsPacked(bool isPacked) {
    _isPacked = isPacked;
    _isPackedDirty = false;
}

void FontShader::setHasOutline(bool hasOutline) {
	_hasOutline = hasOutline;
	_hasOutlineDirty = false;
}

void FontShader::setPage(short page) {
    _page = page;
    _pageDirty = false;
}

void FontShader::setChannel(short channel) {
    _channel = channel;
    _channelDirty = false;
}

void FontShader::setTextColor(const glm::vec4 &color) {
	_textColor = color;
	_textColorDirty = false;
}

void FontShader::setOutlineColor(const glm::vec4 &color) {
	_outlineColor = color;
	_outlineColorDirty = false;
}

void FontShader::setMode(FontRenderMode m) {
	_mode = static_cast<int>(m);
	_modeDirty = false;
}

ShaderManager::ShaderManager()
{
	_programs.resize(NUM_PROGRAMS);
	_programs[DEFAULT_PROGRAM] = std::unique_ptr<DefaultShader>(new DefaultShader(this));
	_programs[BLOCK_PROGRAM] = std::unique_ptr<BlockShader>(new BlockShader(this));
	_programs[HUD_PROGRAM] = std::unique_ptr<HudShader>(new HudShader(this));
	_programs[FONT_PROGRAM] = std::unique_ptr<FontShader>(new FontShader(this));
}

DefaultShader &ShaderManager::getDefaultShader() {
	return static_cast<DefaultShader &>(*_programs[DEFAULT_PROGRAM]);
}

BlockShader &ShaderManager::getBlockShader() {
	return static_cast<BlockShader &>(*_programs[BLOCK_PROGRAM]);
}

HudShader &ShaderManager::getHudShader() {
	return static_cast<HudShader &>(*_programs[HUD_PROGRAM]);
}

FontShader &ShaderManager::getFontShader() {
	return static_cast<FontShader &>(*_programs[FONT_PROGRAM]);
}

void ShaderManager::useProgram(GLuint program) {
	if (_activeProgram != program) {
		glUseProgram(program);
		_activeProgram = program;
	}
}
