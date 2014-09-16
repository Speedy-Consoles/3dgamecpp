/*
 * menu.hpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#ifndef MENU_HPP_
#define MENU_HPP_

#include "config.hpp"

class Graphics;

#include <string>

class Menu {
public:
	Menu() = delete;
	Menu(Graphics *);

	uint getCursor() const { return cursor; }

	void navigateUp();
	void navigateDown();
	void navigateRight();
	void navigateLeft();

	void reload();
	void flush();

	uint getEntryCount();
	std::string getEntryName(uint);
	std::string getEntryValue(uint);

private:
	Graphics *graphics = nullptr;
	uint cursor = 0;

	GraphicsConf conf;
};

#endif // MENU_HPP_
