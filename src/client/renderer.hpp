#ifndef RENDERER_HPP
#define RENDERER_HPP

struct GraphicsConf;

class Renderer {
public:
	virtual ~Renderer() = default;

	virtual void tick() = 0;
	virtual void resize() = 0;
	virtual void setConf(const GraphicsConf &, const GraphicsConf &) = 0;
};

#endif // RENDERER_HPP
