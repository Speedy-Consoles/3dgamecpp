#include "widget.hpp"

using namespace gui;

float Widget::x() const {
	if (_parent)
		return _x + _parent->_x;
	else
		return _x;
}
float Widget::y() const {
	if (_parent)
		return _y + _parent->_y;
	else
		return _y;
}

void Widget::add(Widget *widget) {
	_children.push_back(widget);
	widget->_parent = this;
}

void Widget::updateMousePosition(float x, float y) {
	_hover = isInside(x, y);

	for (Widget *widget : _children) {
		widget->updateMousePosition(x - _x, y - _y);
	}
}

void Widget::handleMouseClick(float x, float y) {
	for (Widget *widget : _children) {
		widget->handleMouseClick(x - _x, y - _y);
	}
}

bool Widget::isInside(float x, float y) {
	return _x <= x && x < _x + _w && _y <= y && y < _y + _h;
}