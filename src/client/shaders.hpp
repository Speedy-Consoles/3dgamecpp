#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

enum ShaderProgram {
	DEFAULT_PROGRAM,
	BLOCK_PROGRAM,
	HUD_PROGRAM,
    FONT_PROGRAM,
};

static const int NUM_PROGRAMS = 4;

class Shaders {

	// program locations
	GLuint programLocations[NUM_PROGRAMS];

	// uniform locations
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

    GLint hudProjMatLoc;

    GLint fontProjMatLoc;
    GLint fontModelMatLoc;
    GLint fontTexLoc;
    GLint fontIsPackedLoc;
    GLint fontPageLoc;
    GLint fontChannelLoc;

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
    GLshort fontPage;
    GLshort fontChannel;

	GLboolean fogEnabled;
	GLfloat fogStartDistance;
	GLfloat fogEndDistance;

	// uniform up-to-dateness
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

	bool hudProjMatUp = false;

    bool fontProjMatUp = false;
    bool fontModelMatUp = false;
    bool fontIsPackedUp = false;
    bool fontChannelUp = false;
    bool fontPageUp = false;

	// active program
	ShaderProgram activeProgram;

public:
	Shaders();
	~Shaders();

	void setLightEnabled(bool enabled);
	void setDiffuseLightColor(const glm::vec3 &color);
	void setDiffuseLightDirection(const glm::vec3 &direction);
	void setAmbientLightColor(const glm::vec3 &color);

	void setModelMatrix(const glm::mat4 &matrix);
	void setViewMatrix(const glm::mat4 &matrix);
	void setProjectionMatrix(const glm::mat4 &matrix);

    void setHudProjectionMatrix(const glm::mat4 &matrix);

    void setFontProjectionMatrix(const glm::mat4 &matrix);
    void setFontModelMatrix(const glm::mat4 &matrix);
    void setFontIsPacked(bool isPacked);
    void setFontPage(short page);
    void setFontChannel(short channel);

	void setFogEnabled(bool enabled);
	void setStartFogDistance(float distance);
	void setEndFogDistance(float distance);

	void prepareProgram(ShaderProgram program);
private:
	void buildShader(GLuint shaderLoc, const char* fileName);
	void buildProgram(GLuint programLoc, GLuint *shaders, int numShaders);
};

#endif /* SHADERS_HPP */