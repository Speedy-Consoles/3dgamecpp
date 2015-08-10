#ifndef WIDGET_HPP_
#define WIDGET_HPP_

namespace gui {

class Widget {
public:
	virtual ~Widget() = default;
	Widget() = default;
	Widget(float x, float y, float w, float h);

	float x() const { return _x; }
	float y() const { return _y; }
	float width() const { return _w; }
	float height() const { return _h; }

	bool hover() const { return _hover; }

	virtual void updateMousePosition(float x, float y);
	virtual void handleMouseClick(float x, float y);

protected:
	float _x = -1, _y = -1, _w = -1, _h = -1;
	bool _hover = false;

	bool isInside(float x, float y);
};

} // namespace gui

#endif // WIDGET_HPP_
