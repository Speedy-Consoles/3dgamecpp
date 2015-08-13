#ifndef GL3_MENU_RENDERER_HPP_
#define GL3_MENU_RENDERER_HPP_

#include <memory>

#include "client/gfx/component_renderer.hpp"

class GL3Renderer;
class Client;
class ShaderManager;
class Graphics;

class GL3MenuRendererImpl;

class GL3MenuRenderer : public ComponentRenderer {
public:
	~GL3MenuRenderer();
	GL3MenuRenderer(Client *client, GL3Renderer *renderer);

	void render() override;

private:
	// pimpl idiom
	std::unique_ptr<GL3MenuRendererImpl> impl;
};

#endif //GL3_MENU_RENDERER_HPP_
