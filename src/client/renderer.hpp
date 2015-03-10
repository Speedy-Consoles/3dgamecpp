#ifndef RENDERER_HPP
#define RENDERER_HPP

struct GraphicsConf;

class Renderer {
public:
	virtual ~Renderer() = default;

	virtual void tick() = 0;

	virtual void resize(int width, int height) = 0;

	virtual void setDebug(bool debugActive) = 0;
	virtual bool isDebug() = 0;

	virtual const GraphicsConf &getConf() const = 0;
	virtual void setConf(const GraphicsConf &) = 0;
};

#endif // RENDERER_HPP
