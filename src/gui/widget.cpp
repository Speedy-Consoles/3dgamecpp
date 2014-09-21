/*
 * widget.cpp
 *
 *  Created on: 21.09.2014
 *      Author: lars
 */

#include "widget.hpp"

using namespace gui;

Widget::Widget(float x, float y, float w, float h) :
	_x(x), _y(y), _w(w), _h(h)
{
	// nothing
}

void Widget::updateMousePosition(float x, float y) {
	_hover = isInside(x, y);
}

void Widget::handleMouseClick(float x, float y) {
	// nothing
}

bool Widget::isInside(float x, float y) {
	return _x <= x && x < _x + _w && _y <= y && y < _y + _h;
}
