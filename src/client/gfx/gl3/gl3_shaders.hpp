#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <vector>

enum ShaderProgram {
	DEFAULT_PROGRAM,
	BLOCK_PROGRAM,
	HUD_PROGRAM,
    FONT_PROGRAM,
};

static const int NUM_PROGRAMS = 4;

class ShaderManager;

class Shader {
	ShaderManager *_manager = nullptr;
	GLuint _programLocation;

public:
	Shader(ShaderManager *, const char *, const char *);
	virtual ~Shader();

	GLuint getProgramLocation() const { return _programLocation; }
	GLuint getUniformLocation(const char *name) const;
	void useProgram();

private:
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);
};

class DefaultShader : public Shader {
	GLint _lightEnabledLoc;
	GLint _ambientColorLoc;
	GLint _diffuseColorLoc;
	GLint _diffuseDirectionLoc;
	GLint _modelMatrixLoc;
	GLint _viewMatrixLoc;
	GLint _projectionMatrixLoc;
	GLint _fogEnabledLoc;
	GLint _fogStartDistanceLoc;
	GLint _fogEndDistanceLoc;

	bool _lightEnabledDirty = true;
	bool _ambientColorDirty = true;
	bool _diffuseColorDirty = true;
	bool _diffuseDirectionDirty = true;
	bool _modelMatrixDirty = true;
	bool _viewMatrixDirty = true;
	bool _projectionMatrixDirty = true;
	bool _fogEnabledDirty = true;
	bool _fogStartDistanceDirty = true;
	bool _fogEndDistanceDirty = true;

	GLboolean _lightEnabled = GL_FALSE;
	glm::vec3 _ambientColor = {1.0, 1.0, 1.0};
	glm::vec3 _diffuseColor = {1.0, 1.0, 1.0};
	glm::vec3 _diffuseDirection = {0.0, 0.0, -1.0};

	glm::mat4 _modelMatrix = glm::mat4();
	glm::mat4 _viewMatrix = glm::mat4();
	glm::mat4 _projectionMatrix = glm::mat4();

	GLboolean _fogEnabled = GL_FALSE;
	GLfloat _fogStartDistance = 0;
	GLfloat _fogEndDistance = 0;

public:
	DefaultShader(ShaderManager *);
	virtual ~DefaultShader() = default;

	void useProgram();

	void setLightEnabled(bool enabled);
	void setDiffuseLightColor(const glm::vec3 &color);
	void setDiffuseLightDirection(const glm::vec3 &direction);
	void setAmbientLightColor(const glm::vec3 &color);

	void setModelMatrix(const glm::mat4 &matrix);
	void setViewMatrix(const glm::mat4 &matrix);
	void setProjectionMatrix(const glm::mat4 &matrix);

	void setFogEnabled(bool enabled);
	void setStartFogDistance(float distance);
	void setEndFogDistance(float distance);
};

class BlockShader : public Shader {
	GLint _lightEnabledLoc;
	GLint _ambientColorLoc;
	GLint _diffuseColorLoc;
	GLint _diffuseDirectionLoc;
	GLint _modelMatrixLoc;
	GLint _viewMatrixLoc;
	GLint _projectionMatrixLoc;
	GLint _fogEnabledLoc;
	GLint _fogStartDistanceLoc;
	GLint _fogEndDistanceLoc;

	bool _lightEnabledDirty = true;
	bool _ambientColorDirty = true;
	bool _diffuseColorDirty = true;
	bool _diffuseDirectionDirty = true;
	bool _modelMatrixDirty = true;
	bool _viewMatrixDirty = true;
	bool _projectionMatrixDirty = true;
	bool _fogEnabledDirty = true;
	bool _fogStartDistanceDirty = true;
	bool _fogEndDistanceDirty = true;

	GLboolean _lightEnabled = GL_FALSE;
	glm::vec3 _ambientColor = {1.0, 1.0, 1.0};
	glm::vec3 _diffuseColor = {1.0, 1.0, 1.0};
	glm::vec3 _diffuseDirection = {0.0, 0.0, -1.0};

	glm::mat4 _modelMatrix = glm::mat4();
	glm::mat4 _viewMatrix = glm::mat4();
	glm::mat4 _projectionMatrix = glm::mat4();

	GLboolean _fogEnabled = GL_FALSE;
	GLfloat _fogStartDistance = 0;
	GLfloat _fogEndDistance = 0;

public:
	BlockShader(ShaderManager *);
	virtual ~BlockShader() = default;

	void useProgram();

	void setLightEnabled(bool enabled);
	void setDiffuseLightColor(const glm::vec3 &color);
	void setDiffuseLightDirection(const glm::vec3 &direction);
	void setAmbientLightColor(const glm::vec3 &color);

	void setModelMatrix(const glm::mat4 &matrix);
	void setViewMatrix(const glm::mat4 &matrix);
	void setProjectionMatrix(const glm::mat4 &matrix);

	void setFogEnabled(bool enabled);
	void setStartFogDistance(float distance);
	void setEndFogDistance(float distance);
};

class HudShader : public Shader {
	GLint _projectionMatrixLoc;
	
	bool _projectionMatrixDirty = true;
	
	glm::mat4 _projectionMatrix = glm::mat4();

public:
	HudShader(ShaderManager *);
	virtual ~HudShader() = default;
	
	void useProgram();
	
    void setProjectionMatrix(const glm::mat4 &matrix);
};

class FontShader : public Shader {
    GLint _mvpMatrixLoc;
	GLint _texLoc;
	GLint _isPackedLoc;
	GLint _hasOutlineLoc;
    GLint _pageLoc;
	GLint _channelLoc;
	GLint _textColorLoc;
	GLint _outlineColorLoc;
	GLint _modeLoc;

    glm::mat4 _projectionMatrix = glm::mat4();
	glm::mat4 _modelMatrix = glm::mat4();
	GLboolean _isPacked = GL_FALSE;
	GLboolean _hasOutline = GL_FALSE;
    GLshort _page = 0;
	GLshort _channel = 0;
	glm::vec4 _textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);;
	glm::vec4 _outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	GLshort _mode = 0;

    bool _projectionMatrixDirty = true;
	bool _modelMatrixDirty = true;
	bool _isPackedDirty = true;
	bool _hasOutlineDirty = true;
    bool _channelDirty = true;
	bool _pageDirty = true;
	bool _textColorDirty = true;
	bool _outlineColorDirty = true;
	bool _modeDirty = true;

public:
	FontShader(ShaderManager *);
	virtual ~FontShader() = default;
	
	void useProgram();

    void setProjectionMatrix(const glm::mat4 &matrix);
	void setModelMatrix(const glm::mat4 &matrix);
	void setIsPacked(bool isPacked);
	void setHasOutline(bool hasOutline);
    void setPage(short page);
	void setChannel(short channel);
	void setTextColor(const glm::vec4 &color);
	void setOutlineColor(const glm::vec4 &color);
	enum class FontRenderMode { DEFAULT, OUTLINE, TEXT };
	void setMode(FontRenderMode mode);
};

class ShaderManager {
	std::vector<std::unique_ptr<Shader>> _programs;
	GLuint _activeProgram;

public:
	ShaderManager();
	
	DefaultShader &getDefaultShader();
	BlockShader &getBlockShader();
	HudShader &getHudShader();
	FontShader &getFontShader();

	void useProgram(GLuint);
};

#endif /* SHADERS_HPP */
