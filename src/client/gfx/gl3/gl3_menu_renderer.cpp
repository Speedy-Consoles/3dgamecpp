#include "gl3_menu_renderer.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shared/engine/logging.hpp"

#include "client/gfx/graphics.hpp"
#include "client/gfx/gl3/gl3_renderer.hpp"
#include "client/gfx/gl3/gl3_shaders.hpp"
#include "client/gui/widget.hpp"
#include "client/gui/label.hpp"
#include "client/gui/button.hpp"

#include "gl3_font.hpp"

using namespace gui;

class GL3MenuRendererImpl {
public:
	~GL3MenuRendererImpl();
	GL3MenuRendererImpl(Client *client, GL3Renderer *renderer);
	void render();

private:
	Client *client = nullptr;
	GL3Renderer *renderer = nullptr;

	BMFont font;

	void renderWidget(const gui::Widget *);
	void renderFrame(const gui::Frame *);
	void renderLabel(const gui::Label *);
	void renderButton(const gui::Button *);
	void renderText(const char *text, float x, float y);

	GLuint square_vao = 0;
	GLuint square_vbo = 0;
};

GL3MenuRenderer::~GL3MenuRenderer() = default;

GL3MenuRenderer::GL3MenuRenderer(Client *client, GL3Renderer *renderer) :
	impl(new GL3MenuRendererImpl(client, renderer))
{
	// nothing
}

void GL3MenuRenderer::render() {
	impl->render();
}

// PIMPL

GL3MenuRendererImpl::~GL3MenuRendererImpl() {
	GL(DeleteVertexArrays(1, &square_vao));
	GL(DeleteBuffers(1, &square_vbo));
}

GL3MenuRendererImpl::GL3MenuRendererImpl(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer),
	font(&renderer->getShaderManager()->getFontShader())
{
	GL(GenVertexArrays(1, &square_vao));
	GL(GenBuffers(1, &square_vbo));
	GL(BindVertexArray(square_vao));
	GL(BindBuffer(GL_ARRAY_BUFFER, square_vbo));
	GL(VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, (void *) 0));
	GL(VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (void *) 8));
	GL(EnableVertexAttribArray(0));
	GL(EnableVertexAttribArray(1));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
	GL(BindVertexArray(0));

	static float square[36] = {
		0, 0, 1, 1, 1, 1,
		1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
	};

	GL(BindBuffer(GL_ARRAY_BUFFER, square_vbo));
	GL(BufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW));
	GL(BindBuffer(GL_ARRAY_BUFFER, 0));
	
	font.load("fonts/dejavusansmono20.fnt");
	font.setEncoding(Font::Encoding::UTF8);
}

void GL3MenuRendererImpl::render() {
	if (client->getState() == Client::State::IN_MENU)
		renderWidget((const Widget *) client->getMenu()->getFrame());
}

void GL3MenuRendererImpl::renderWidget(const Widget *widget) {
	const Label *label = nullptr;
	const Button *button = nullptr;

	HudShader &hud_shader = renderer->getShaderManager()->getHudShader();
	glm::mat4 model;
	model = glm::translate(model, glm::vec3{ widget->x(), widget->y(), 0 });
	model = glm::scale(model, glm::vec3{ widget->width(), widget->height(), 1 });
	hud_shader.setModelMatrix(model);
	hud_shader.setColor(glm::vec4(0, 0, 0, 0.2));
	hud_shader.useProgram();

	GL(BindVertexArray(square_vao));
	GL(DrawArrays(GL_TRIANGLES, 0, 6));

	if ((button = dynamic_cast<const Button *>(widget))) {
		renderButton(button);
	} else if ((label = dynamic_cast<const Label *>(widget))) {
		renderLabel(label);
	}

	for (const Widget *child : widget->children()) {
		renderWidget(child);
	}
}

void GL3MenuRendererImpl::renderLabel(const Label *label) {
	glm::vec4 normal(1.0, 1.0, 1.0, 1.0);
	font.setColor(normal);
	renderText(label->text().c_str(), label->x(), label->y());
}

void GL3MenuRendererImpl::renderButton(const Button *button) {
	glm::vec4 highlight(1.0, 1.0, 0.0, 1.0);
	glm::vec4 normal(1.0, 1.0, 1.0, 1.0);
	if (button->hover()) {
		font.setColor(highlight);
	} else {
		font.setColor(normal);
	}

	renderText(button->text().c_str(), button->x(), button->y());
	font.setColor(normal);
}

void GL3MenuRendererImpl::renderText(const char *text, float x, float y) {
	font.write(x + 5, y - font.getBottomOffset(), 0.0f, text, 0);
}
