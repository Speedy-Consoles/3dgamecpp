#include "gl3_shaders.hpp"

#include <fstream>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"

static logging::Logger logger("gfx");

Shader::Shader(GL3ShaderManager *manager, const char *vert_src, const char *frag_src) :
	_manager(manager)
{
	GLuint VertexShaderLoc = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderLoc = glCreateShader(GL_FRAGMENT_SHADER);
	LOG_TRACE(logger) << "Building '" << vert_src << "'";
	buildShader(VertexShaderLoc, vert_src);
	LOG_TRACE(logger) << "Building '" << frag_src << "'";
	buildShader(FragmentShaderLoc, frag_src);

	_programLocation = glCreateProgram();
	GLuint ProgramShaderLocs[2] = {VertexShaderLoc, FragmentShaderLoc};

	LOG_TRACE(logger) << "Building program";
	buildProgram(_programLocation, ProgramShaderLocs, 2);

	glDeleteShader(VertexShaderLoc);
	glDeleteShader(FragmentShaderLoc);
	
	LOG_OPENGL_ERROR;
}

Shader::~Shader() {
	glDeleteProgram(_programLocation);
}

GLuint Shader::getUniformLocation(const char *name) const {
	GLuint loc = glGetUniformLocation(_programLocation, name);
	LOG_OPENGL_ERROR;
	return loc;
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
		LOG_FATAL(logger) << "Could not open file!";
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
		LOG_ERROR(logger) << &shaderErrorMessage[0];
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
		LOG_ERROR(logger) << &programErrorMessage[0];
}

DefaultShader::DefaultShader(GL3ShaderManager *manager) :
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

	if (_lightEnabledDirty) {
		glUniform1i(_lightEnabledLoc, _lightEnabled);
		_lightEnabledDirty = false;
	}
	if (_ambientColorDirty) {
		glUniform3fv(_ambientColorLoc, 1, glm::value_ptr(_ambientColor));
		_ambientColorDirty = false;
	}
	if (_diffuseColorDirty) {
		glUniform3fv(_diffuseColorLoc, 1, glm::value_ptr(_diffuseColor));
		_diffuseColorDirty = false;
	}
	if (_diffuseDirectionDirty) {
		glUniform3fv(_diffuseDirectionLoc, 1, glm::value_ptr(_diffuseDirection));
		_diffuseDirectionDirty = false;
	}
	if (_modelMatrixDirty) {
		glUniformMatrix4fv(_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(_modelMatrix));
		_modelMatrixDirty = false;
	}
	if (_viewMatrixDirty) {
		glUniformMatrix4fv(_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(_viewMatrix));
		_viewMatrixDirty = false;
	}
	if (_projectionMatrixDirty) {
		glUniformMatrix4fv(_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
		_projectionMatrixDirty = false;
	}
	if (_fogEnabledDirty) {
		glUniform1i(_fogEnabledLoc, _fogEnabled);
		_fogEnabledDirty = false;
	}
	if (_fogStartDistanceDirty) {
		glUniform1f(_fogStartDistanceLoc, _fogStartDistance);
		_fogStartDistanceDirty = false;
	}
	if (_fogEndDistanceDirty) {
		glUniform1f(_fogEndDistanceLoc, _fogEndDistance);
		_fogEndDistanceDirty = false;
	}
}

void DefaultShader::setLightEnabled(bool enabled) {
	GLboolean b = enabled ? GL_TRUE : GL_FALSE;
	if (_lightEnabled != b) {
		_lightEnabled = b;
		_lightEnabledDirty = true;
	}
}

void DefaultShader::setDiffuseLightColor(const glm::vec3 &color) {
	if (_diffuseColor != color) {
		_diffuseColor = color;
		_diffuseColorDirty = true;
	}
}

void DefaultShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	if (_diffuseDirection != direction) {
		_diffuseDirection = direction;
		_diffuseDirectionDirty = true;
	}
}

void DefaultShader::setAmbientLightColor(const glm::vec3 &color) {
	if (_ambientColor != color) {
		_ambientColor = color;
		_ambientColorDirty = true;
	}
}

void DefaultShader::setModelMatrix(const glm::mat4 &matrix) {
	if (_modelMatrix != matrix) {
		_modelMatrix = matrix;
		_modelMatrixDirty = true;
	}
}

void DefaultShader::setViewMatrix(const glm::mat4 &matrix) {
	if (_viewMatrix != matrix) {
		_viewMatrix = matrix;
		_viewMatrixDirty = true;
	}
}

void DefaultShader::setProjectionMatrix(const glm::mat4 &matrix) {
	if (_projectionMatrix != matrix) {
		_projectionMatrix = matrix;
		_projectionMatrixDirty = true;
	}
}

void DefaultShader::setFogEnabled(bool enabled) {
	GLboolean b = enabled ? GL_TRUE : GL_FALSE;
	if (_fogEnabled != b) {
		_fogEnabled = b;
		_fogEnabledDirty = true;
	}
}

void DefaultShader::setStartFogDistance(float distance) {
	if (_fogStartDistance != distance) {
		_fogStartDistance = distance;
		_fogStartDistanceDirty = true;
	}
}

void DefaultShader::setEndFogDistance(float distance) {
	if (_fogEndDistance != distance) {
		_fogEndDistance = distance;
		_fogEndDistanceDirty = true;
	}
}

BlockShader::BlockShader(GL3ShaderManager *manager) :
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

	if (_lightEnabledDirty) {
		glUniform1i(_lightEnabledLoc, _lightEnabled);
		_lightEnabledDirty = false;
	}
	if (_ambientColorDirty) {
		glUniform3fv(_ambientColorLoc, 1, glm::value_ptr(_ambientColor));
		_ambientColorDirty = false;
	}
	if (_diffuseColorDirty) {
		glUniform3fv(_diffuseColorLoc, 1, glm::value_ptr(_diffuseColor));
		_diffuseColorDirty = false;
	}
	if (_diffuseDirectionDirty) {
		glUniform3fv(_diffuseDirectionLoc, 1, glm::value_ptr(_diffuseDirection));
		_diffuseDirectionDirty = false;
	}
	if (_modelMatrixDirty) {
		glUniformMatrix4fv(_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(_modelMatrix));
		_modelMatrixDirty = false;
	}
	if (_viewMatrixDirty) {
		glUniformMatrix4fv(_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(_viewMatrix));
		_viewMatrixDirty = false;
	}
	if (_projectionMatrixDirty) {
		glUniformMatrix4fv(_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(_projectionMatrix));
		_projectionMatrixDirty = false;
	}
	if (_fogEnabledDirty) {
		glUniform1i(_fogEnabledLoc, _fogEnabled);
		_fogEnabledDirty = false;
	}
	if (_fogStartDistanceDirty) {
		glUniform1f(_fogStartDistanceLoc, _fogStartDistance);
		_fogStartDistanceDirty = false;
	}
	if (_fogEndDistanceDirty) {
		glUniform1f(_fogEndDistanceLoc, _fogEndDistance);
		_fogEndDistanceDirty = false;
	}
}

void BlockShader::setLightEnabled(bool enabled) {
	GLboolean b = enabled ? GL_TRUE : GL_FALSE;
	if (_lightEnabled != b) {
		_lightEnabled = b;
		_lightEnabledDirty = true;
	}
}

void BlockShader::setDiffuseLightColor(const glm::vec3 &color) {
	if (_diffuseColor != color) {
		_diffuseColor = color;
		_diffuseColorDirty = true;
	}
}

void BlockShader::setDiffuseLightDirection(const glm::vec3 &direction) {
	if (_diffuseDirection != direction) {
		_diffuseDirection = direction;
		_diffuseDirectionDirty = true;
	}
}

void BlockShader::setAmbientLightColor(const glm::vec3 &color) {
	if (_ambientColor != color) {
		_ambientColor = color;
		_ambientColorDirty = true;
	}
}

void BlockShader::setModelMatrix(const glm::mat4 &matrix) {
	if (_modelMatrix != matrix) {
		_modelMatrix = matrix;
		_modelMatrixDirty = true;
	}
}

void BlockShader::setViewMatrix(const glm::mat4 &matrix) {
	if (_viewMatrix != matrix) {
		_viewMatrix = matrix;
		_viewMatrixDirty = true;
	}
}

void BlockShader::setProjectionMatrix(const glm::mat4 &matrix) {
	if (_projectionMatrix != matrix) {
		_projectionMatrix = matrix;
		_projectionMatrixDirty = true;
	}
}

void BlockShader::setFogEnabled(bool enabled) {
	GLboolean b = enabled ? GL_TRUE : GL_FALSE;
	if (_fogEnabled != b) {
		_fogEnabled = b;
		_fogEnabledDirty = true;
	}
}

void BlockShader::setStartFogDistance(float distance) {
	if (_fogStartDistance != distance) {
		_fogStartDistance = distance;
		_fogStartDistanceDirty = true;
	}
}

void BlockShader::setEndFogDistance(float distance) {
	if (_fogEndDistance != distance) {
		_fogEndDistance = distance;
		_fogEndDistanceDirty = true;
	}
}

HudShader::HudShader(GL3ShaderManager *manager) :
	Shader(manager, "shaders/hud.vert", "shaders/hud.frag")
{
	_mvpMatrixLoc = getUniformLocation("projectionMatrix");
	_colorLoc = getUniformLocation("diffuseColor");
	LOG_OPENGL_ERROR;
}

void HudShader::useProgram() {
	Shader::useProgram();
	
	if (_projectionMatrixDirty || _modelMatrixDirty) {
		glm::mat4 mvpMatrix = _projectionMatrix * _modelMatrix;
		GL(UniformMatrix4fv(_mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix)));
		_projectionMatrixDirty = false;
		_modelMatrixDirty = false;
	}

	if (_colorDirty) {
		GL(Uniform4fv(_colorLoc, 1, glm::value_ptr(_color)));
		_colorDirty = false;
	}
}

void HudShader::setProjectionMatrix(const glm::mat4 &matrix) {
	if (_projectionMatrix != matrix) {
		_projectionMatrix = matrix;
		_projectionMatrixDirty = true;
	}
}

void HudShader::setModelMatrix(const glm::mat4 &matrix) {
	if (_modelMatrix != matrix) {
		_modelMatrix = matrix;
		_modelMatrixDirty = true;
	}
}

void HudShader::setColor(const glm::vec4 &color) {
	if (_color != color) {
		_color = color;
		_colorDirty = true;
	}
}

FontShader::FontShader(GL3ShaderManager *manager) :
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

	if (_projectionMatrixDirty || _modelMatrixDirty) {
		auto mvpMatrix = _projectionMatrix * _modelMatrix;
		glUniformMatrix4fv(_mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        _projectionMatrixDirty = false;
		_modelMatrixDirty = false;
    }
	if (_isPackedDirty) {
		glUniform1i(_isPackedLoc, _isPacked);
		_isPackedDirty = false;
	}
	if (_hasOutlineDirty) {
		glUniform1i(_hasOutlineLoc, _hasOutline);
		_hasOutlineDirty = false;
	}
    if (_pageDirty) {
        glUniform1i(_pageLoc, _page);
        _pageDirty = false;
	}
	if (_channelDirty) {
		glUniform1i(_channelLoc, _channel);
		_channelDirty = false;
	}
	if (_textColorDirty) {
		glUniform4fv(_textColorLoc, 1, glm::value_ptr(_textColor));
		_textColorDirty = false;
	}
	if (_outlineColorDirty) {
		glUniform4fv(_outlineColorLoc, 1, glm::value_ptr(_outlineColor));
		_outlineColorDirty = false;
	}
	if (_modeDirty) {
		glUniform1i(_modeLoc, _mode);
		_modeDirty = false;
	}
}

void FontShader::setProjectionMatrix(const glm::mat4 &matrix) {
	if (_projectionMatrix != matrix) {
		_projectionMatrix = matrix;
		_projectionMatrixDirty = true;
	}
}

void FontShader::setModelMatrix(const glm::mat4 &matrix) {
	if (_modelMatrix != matrix) {
		_modelMatrix = matrix;
		_modelMatrixDirty = true;
	}
}

void FontShader::setIsPacked(bool isPacked) {
	GLboolean b = isPacked ? GL_TRUE : GL_FALSE;
	if (_isPacked != b) {
		_isPacked = b;
		_isPackedDirty = true;
	}
}

void FontShader::setHasOutline(bool hasOutline) {
	GLboolean b = hasOutline ? GL_TRUE : GL_FALSE;
	if (_hasOutline != b) {
		_hasOutline = b;
		_hasOutlineDirty = true;
	}
}

void FontShader::setPage(short page) {
	if (_page != page) {
		_page = page;
		_pageDirty = true;
	}
}

void FontShader::setChannel(short channel) {
	if (_channel != channel) {
		_channel = channel;
		_channelDirty = true;
	}
}

void FontShader::setTextColor(const glm::vec4 &color) {
	if (_textColor != color) {
		_textColor = color;
		_textColorDirty = true;
	}
}

void FontShader::setOutlineColor(const glm::vec4 &color) {
	if (_outlineColor != color) {
		_outlineColor = color;
		_outlineColorDirty = true;
	}
}

void FontShader::setMode(FontRenderMode m) {
	auto mode = static_cast<int>(m);
	if (_mode != mode) {
		_mode = mode;
		_modeDirty = true;
	}
}

GL3ShaderManager::GL3ShaderManager()
{
	_programs.resize(NUM_PROGRAMS);
	_programs[DEFAULT_PROGRAM] = std::unique_ptr<DefaultShader>(new DefaultShader(this));
	_programs[BLOCK_PROGRAM] = std::unique_ptr<BlockShader>(new BlockShader(this));
	_programs[HUD_PROGRAM] = std::unique_ptr<HudShader>(new HudShader(this));
	_programs[FONT_PROGRAM] = std::unique_ptr<FontShader>(new FontShader(this));
}

DefaultShader &GL3ShaderManager::getDefaultShader() {
	return static_cast<DefaultShader &>(*_programs[DEFAULT_PROGRAM]);
}

BlockShader &GL3ShaderManager::getBlockShader() {
	return static_cast<BlockShader &>(*_programs[BLOCK_PROGRAM]);
}

HudShader &GL3ShaderManager::getHudShader() {
	return static_cast<HudShader &>(*_programs[HUD_PROGRAM]);
}

FontShader &GL3ShaderManager::getFontShader() {
	return static_cast<FontShader &>(*_programs[FONT_PROGRAM]);
}

void GL3ShaderManager::useProgram(GLuint program) {
	if (_activeProgram != program) {
		glUseProgram(program);
		_activeProgram = program;
	}
}
