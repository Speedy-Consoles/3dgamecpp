/*
 * button.cpp
 *
 *  Created on: 21.09.2014
 *      Author: lars
 */

#include "button.hpp"
#include "logging.hpp"

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("gui")

using namespace gui;

Button::Button(float x, float y, float w, float h, const std::string &text) :
	Label(x, y, w, h, text)
{
	// nothing
}

void Button::setOnClick(std::function<void()> on_click) {
	_on_click = on_click;
}

void Button::handleMouseClick(float x, float y) {
	if (isInside(x, y) && _on_click) {
		_on_click();
	}
}
