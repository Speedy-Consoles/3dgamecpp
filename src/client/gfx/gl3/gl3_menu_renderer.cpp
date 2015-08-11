#include "gl3_menu_renderer.hpp"

#include "client/gfx/graphics.hpp"
#include "client/gfx/gl3/gl3_renderer.hpp"
#include "client/gui/widget.hpp"
#include "client/gui/frame.hpp"
#include "client/gui/label.hpp"
#include "client/gui/button.hpp"

#include "gl3_font.hpp"

using namespace gui;

class GL3MenuRendererImpl {
public:
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

GL3MenuRendererImpl::GL3MenuRendererImpl(Client *client, GL3Renderer *renderer) :
	client(client),
	renderer(renderer),
	font(&((GL3Renderer *) renderer)->getShaderManager()->getFontShader())
{
	font.load("fonts/dejavusansmono20.fnt");
	font.setEncoding(Font::Encoding::UTF8);
}

void GL3MenuRendererImpl::render() {
	renderWidget((const Widget *) client->getMenu()->getFrame());
}


void GL3MenuRendererImpl::renderWidget(const Widget *widget) {
	const Label *label = nullptr;
	const Frame *frame = nullptr;
	const Button *button = nullptr;

	if ((button = dynamic_cast<const Button *>(widget))) {
		renderButton(button);
	} else if ((label = dynamic_cast<const Label *>(widget))) {
		renderLabel(label);
	} else if ((frame = dynamic_cast<const Frame *>(widget))) {
		renderFrame(frame);
	}
}

void GL3MenuRendererImpl::renderFrame(const Frame *frame) {
	for (const Widget *widget : frame->widgets()) {
		renderWidget(widget);
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
	float x0 = -client->getGraphics()->getDrawWidth() / 2 + 5;
	float y0 = -client->getGraphics()->getDrawHeight() / 2 - font.getBottomOffset() + 5;

	font.write(x0 + x, y0 + y, 0.0f, text, 0);
}
