/*
 * menu.hpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#ifndef MENU_HPP_
#define MENU_HPP_

#include "config.hpp"

namespace gui {
	class Frame;
	class Label;
}

#include <string>

class Menu {
public:
	Menu() = delete;
	Menu(gui::Frame *frame, GraphicsConf *);

	void handleClickFullscreen(gui::Label *label);
	void handleClickAA(gui::Label *label);
	void handleClickFog(gui::Label *label);
	void handleClickRenderDistance(gui::Label *label);

	void apply();
	bool check() { return dirty; dirty = false; }

private:
	bool dirty = false;
	uint renderDistanceBuf;
	AntiAliasing aaBuf;

	gui::Frame *frame;
	GraphicsConf *conf;
};

#endif // MENU_HPP_
