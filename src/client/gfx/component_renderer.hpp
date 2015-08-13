#ifndef COMPONENT_RENDERER_HPP_
#define COMPONENT_RENDERER_HPP_

struct GraphicsConf;

class ComponentRenderer {
public:
	virtual ~ComponentRenderer() = default;

	virtual void setConf(const GraphicsConf &, const GraphicsConf &) {}
	virtual void resize() {}

	virtual void tick() {}
	virtual void render() = 0;
};

#endif // COMPONENT_RENDERER_HPP_
