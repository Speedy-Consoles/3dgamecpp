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

	bool _lightEnabledDirty = false;
	bool _ambientColorDirty = false;
	bool _diffuseColorDirty = false;
	bool _diffuseDirectionDirty = false;
	bool _modelMatrixDirty = false;
	bool _viewMatrixDirty = false;
	bool _projectionMatrixDirty = false;
	bool _fogEnabledDirty = false;
	bool _fogStartDistanceDirty = false;
	bool _fogEndDistanceDirty = false;

	GLboolean _lightEnabled;
	glm::vec3 _ambientColor;
	glm::vec3 _diffuseColor;
	glm::vec3 _diffuseDirection;

	glm::mat4 _modelMatrix;
	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;

	GLboolean _fogEnabled;
	GLfloat _fogStartDistance;
	GLfloat _fogEndDistance;

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

	bool _lightEnabledDirty = false;
	bool _ambientColorDirty = false;
	bool _diffuseColorDirty = false;
	bool _diffuseDirectionDirty = false;
	bool _modelMatrixDirty = false;
	bool _viewMatrixDirty = false;
	bool _projectionMatrixDirty = false;
	bool _fogEnabledDirty = false;
	bool _fogStartDistanceDirty = false;
	bool _fogEndDistanceDirty = false;

	GLboolean _lightEnabled;
	glm::vec3 _ambientColor;
	glm::vec3 _diffuseColor;
	glm::vec3 _diffuseDirection;

	glm::mat4 _modelMatrix;
	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;

	GLboolean _fogEnabled;
	GLfloat _fogStartDistance;
	GLfloat _fogEndDistance;

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
	
	bool _projectionMatrixDirty = false;
	
	glm::mat4 _projectionMatrix;

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

    glm::mat4 _projectionMatrix;
	glm::mat4 _modelMatrix;
	GLboolean _isPacked;
	GLboolean _hasOutline;
    GLshort _page;
	GLshort _channel;
	glm::vec4 _textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);;
	glm::vec4 _outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	GLshort _mode = 0;

    bool _projectionMatrixDirty = false;
	bool _modelMatrixDirty = false;
	bool _isPackedDirty = false;
	bool _hasOutlineDirty = false;
    bool _channelDirty = false;
	bool _pageDirty = false;
	bool _textColorDirty = false;
	bool _outlineColorDirty = false;
	bool _modeDirty = false;

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
