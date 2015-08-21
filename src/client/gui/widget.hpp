#ifndef WIDGET_HPP_
#define WIDGET_HPP_

#include <list>

namespace gui {

class Widget {
public:
	virtual ~Widget() = default;
	Widget() = default;
	Widget(float x, float y, float w, float h) : _x(x), _y(y), _w(w), _h(h) {}
	Widget(Widget *parent) : _parent(parent) {}
	Widget(Widget *p, float x, float y, float w, float h) :
			_parent(p), _x(x), _y(y), _w(w), _h(h) {}

	float x() const;
	float y() const;
	float width() const { return _w; }
	float height() const { return _h; }

	bool hover() const { return _hover; }

	void move(float x, float y) { _x = x; _y = y; }

	const std::list<Widget *> &children() const { return _children; }

	void add(Widget *widget);

	virtual void updateMousePosition(float x, float y);
	virtual void handleMouseClick(float x, float y);

protected:
	Widget *_parent = nullptr;
	std::list<Widget *> _children;
	float _x = -1, _y = -1, _w = -1, _h = -1;
	bool _hover = false;

	bool isInside(float x, float y);
};

} // namespace gui

#endif // WIDGET_HPP_
