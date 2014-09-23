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
	template <typename T> class CycleButton;
}

#include <string>

class Menu {
public:
	~Menu();
	Menu() = delete;
	Menu(GraphicsConf *);

	gui::Frame *getFrame() { return frame; }
	const gui::Frame *getFrame() const { return frame; }

	void apply();
	void update();
	bool check() { return dirty; dirty = false; }

private:
	bool dirty = false;
	uint renderDistanceBuf;
	AntiAliasing aaBuf;

	GraphicsConf *clientConf = nullptr;
	GraphicsConf bufferConf;

	gui::Frame *frame = nullptr;

	gui::CycleButton<bool> *fsButton;
	gui::CycleButton<AntiAliasing> *aaButton;
	gui::CycleButton<Fog> *fogButton;
	gui::CycleButton<int> *rdButton;
	gui::CycleButton<uint> *mipButton;
	gui::CycleButton<TexFiltering> *filtButton;
};

#endif // MENU_HPP_
