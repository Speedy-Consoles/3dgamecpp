#include "frame.hpp"

#include "label.hpp"

using namespace gui;

Frame::~Frame() {
	for (Widget *widget : _widgets) {
		delete widget;
	}
	_widgets.clear();
}

Frame::Frame(float x, float y, float w, float h) :
	Widget(x, y, w, h)
{
	// nothing
}

void Frame::add(Widget *widget) {
	_widgets.push_back(widget);
}

void Frame::updateMousePosition(float x, float y) {
	Widget::updateMousePosition(x, y);

	for (Widget *widget : _widgets) {
		widget->updateMousePosition(x - _x, y - _y);
	}
}

void Frame::handleMouseClick(float x, float y) {
	Widget::handleMouseClick(x, y);

	for (Widget *widget : _widgets) {
		widget->handleMouseClick(x - _x, y - _y);
	}
}
