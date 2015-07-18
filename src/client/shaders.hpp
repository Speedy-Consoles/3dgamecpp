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
	ShaderManager *manager = nullptr;
	GLuint programLocation;

public:
	Shader(ShaderManager *, const char *, const char *);
	virtual ~Shader();

	GLuint getProgramLocation() const { return programLocation; }
	GLuint getUniformLocation(const char *name) const;
	void useProgram();

private:
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);
};

class DefaultShader : public Shader {
	GLint defaultLightEnabledLoc;
	GLint defaultAmbientColorLoc;
	GLint defaultDiffColorLoc;
	GLint defaultDiffDirLoc;
	GLint defaultModelMatLoc;
	GLint defaultViewMatLoc;
	GLint defaultProjMatLoc;
	GLint defaultFogEnabledLoc;
	GLint defaultFogStartDistanceLoc;
	GLint defaultFogEndDistanceLoc;

	bool defaultLightEnabledUp = false;
	bool defaultAmbientColorUp = false;
	bool defaultDiffColorUp = false;
	bool defaultDiffDirUp = false;
	bool defaultModelMatUp = false;
	bool defaultViewMatUp = false;
	bool defaultProjMatUp = false;
	bool defaultFogEnabledUp = false;
	bool defaultFogStartDistanceUp = false;
	bool defaultFogEndDistanceUp = false;

	GLboolean lightEnabled;
	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 diffuseDirection;

	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	GLboolean fogEnabled;
	GLfloat fogStartDistance;
	GLfloat fogEndDistance;

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

class HudShader : public Shader {
	GLint hudProjMatLoc;
	
	bool hudProjMatUp = false;
	
	glm::mat4 hudProjectionMatrix;

public:
	HudShader(ShaderManager *);
	virtual ~HudShader() = default;
	
	void useProgram();
	
    void setHudProjectionMatrix(const glm::mat4 &matrix);
};

class FontShader : public Shader {
    GLint fontTransMatLoc;
	GLint fontTexLoc;
	GLint fontIsPackedLoc;
	GLint fontHasOutlineLoc;
    GLint fontPageLoc;
	GLint fontChannelLoc;
	GLint fontTextColorLoc;
	GLint fontOutlineColorLoc;
	GLint fontModeLoc;

    glm::mat4 fontProjectionMatrix;
	glm::mat4 fontModelMatrix;
	GLboolean fontIsPacked;
	GLboolean fontHasOutline;
    GLshort fontPage;
	GLshort fontChannel;
	glm::vec4 fontTextColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);;
	glm::vec4 fontOutlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	GLshort fontMode = 0;

    bool fontProjMatUp = false;
	bool fontModelMatUp = false;
	bool fontIsPackedUp = false;
	bool fontHasOutlineUp = false;
    bool fontChannelUp = false;
	bool fontPageUp = false;
	bool fontTextColorUp = false;
	bool fontOutlineColorUp = false;
	bool fontModeUp = false;

public:
	FontShader(ShaderManager *);
	virtual ~FontShader() = default;
	
	void useProgram();

    void setFontProjectionMatrix(const glm::mat4 &matrix);
	void setFontModelMatrix(const glm::mat4 &matrix);
	void setFontIsPacked(bool isPacked);
	void setFontHasOutline(bool hasOutline);
    void setFontPage(short page);
	void setFontChannel(short channel);
	void setFontTextColor(const glm::vec4 &color);
	void setFontOutlineColor(const glm::vec4 &color);
	enum class FontRenderMode { DEFAULT, OUTLINE, TEXT };
	void setFontMode(FontRenderMode mode);
};

class Shaders {

	ShaderManager *manager;

	// program locations
	GLuint programLocations[NUM_PROGRAMS];

	// uniform locations

	GLint blockLightEnabledLoc;
	GLint blockAmbientColorLoc;
	GLint blockDiffColorLoc;
	GLint blockDiffDirLoc;
	GLint blockModelMatLoc;
	GLint blockViewMatLoc;
	GLint blockProjMatLoc;
	GLint blockFogEnabledLoc;
	GLint blockFogStartDistanceLoc;
	GLint blockFogEndDistanceLoc;

	// uniforms
	GLboolean lightEnabled;
	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 diffuseDirection;

	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	glm::mat4 hudProjectionMatrix;

    glm::mat4 fontProjectionMatrix;
	glm::mat4 fontModelMatrix;
	GLboolean fontIsPacked;
	GLboolean fontHasOutline;
    GLshort fontPage;
	GLshort fontChannel;
	glm::vec4 fontTextColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);;
	glm::vec4 fontOutlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	GLshort fontMode = 0;

	GLboolean fogEnabled;
	GLfloat fogStartDistance;
	GLfloat fogEndDistance;

	// uniform up-to-dateness
	bool blockLightEnabledUp = false;
	bool blockAmbientColorUp = false;
	bool blockDiffColorUp = false;
	bool blockDiffDirUp = false;
	bool blockModelMatUp = false;
	bool blockViewMatUp = false;
	bool blockProjMatUp = false;
	bool blockFogEnabledUp = false;
	bool blockFogStartDistanceUp = false;
	bool blockFogEndDistanceUp = false;

public:
	Shaders(ShaderManager *manager);
	~Shaders();

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

	void prepareProgram(ShaderProgram program);
private:
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);
};

class ShaderManager {
	Shaders shaders;
	std::vector<std::unique_ptr<Shader>> programs;
	GLuint activeProgram;

public:
	ShaderManager();
	
	Shaders &getShaders();
	DefaultShader &getDefaultShader();
	HudShader &getHudShader();
	FontShader &getFontShader();

	void useProgram(GLuint);
};

#endif /* SHADERS_HPP */
