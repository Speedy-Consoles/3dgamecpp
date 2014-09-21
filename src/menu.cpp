/*
 * menu.cpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#include "menu.hpp"

#include "graphics.hpp"
#include "util.hpp"

#include "gui/frame.hpp"
#include "gui/label.hpp"
#include "gui/button.hpp"

#include <string>

using namespace std;
using namespace gui;

template <typename T>
struct Option {
	T opt;
	const char *desc;
};

Option<bool> fullscreenOptions[] = {
	{false, nullptr},
	{true, "Yes"},
	{false, "No"},
	{false, nullptr},
};

Option<AntiAliasing> aaOptions[] = {
	{AntiAliasing::NONE, nullptr},
	{AntiAliasing::NONE, "Off"},
	{AntiAliasing::MSAA_2, "MSAA x2"},
	{AntiAliasing::MSAA_4, "MSAA x4"},
	{AntiAliasing::MSAA_8, "MSAA x8"},
	{AntiAliasing::MSAA_16, "MSAA x16"},
	{AntiAliasing::NONE, nullptr},
};

Option<Fog> fogOptions[] = {
	{Fog::NONE, nullptr},
	{Fog::NONE, "Off"},
	{Fog::FAST, "Fast"},
	{Fog::FANCY, "Fancy"},
	{Fog::NONE, nullptr},
};

Option<uint> renderDistanceOptions[] = {
	{0, nullptr},
	{4, "4"},
	{8, "8"},
	{12, "12"},
	{16, "16"},
	{24, "24"},
	{32, "32"},
	{0, nullptr},
};

template <typename T>
int getIndex(T t, const Option<T> *options) {
	int i = 1;
	while (options[i].desc != nullptr) {
		if (t == options[i].opt)
			return i;
		++i;
	}
	return -1;
}

template <typename T>
string getName(T t, const Option<T> *options) {
	return options[getIndex(t, options)].desc;
}

Menu::Menu(Frame *frame, GraphicsConf *conf) :
	frame(frame), conf(conf)
{
	renderDistanceBuf = conf->render_distance;
	aaBuf = conf->aa;

	int yIncr = 20;
	int y = yIncr;

	Button *applyButton = new Button(0, y, 100, 20);
	applyButton->text() = string("Apply");
	applyButton->setOnClick([this](){ apply(); });
	frame->add(applyButton);
	y += yIncr * 2;

	Button *fsButton = new Button(0, y, 100, 20);
	fsButton->text() = string("Fullscreen: ") +
			getName(conf->fullscreen, fullscreenOptions);
	fsButton->setOnClick([this, fsButton](){ handleClickFullscreen(fsButton); });
	frame->add(fsButton);
	y += yIncr;

	Button *aaButton = new Button(0, y, 100, 20);
	aaButton->text() = string("Anti-aliasing: ") +
			getName(conf->aa, aaOptions);
	aaButton->setOnClick([this, aaButton](){ handleClickAA(aaButton); });
	frame->add(aaButton);
	y += yIncr;

	Button *fogButton = new Button(0, y, 100, 20);
	fogButton->text() = string("Fog: ") +
			getName(conf->fog, fogOptions);
	fogButton->setOnClick([this, fogButton](){ handleClickFog(fogButton); });
	frame->add(fogButton);
	y += yIncr;

	Button *rdButton = new Button(0, y, 100, 20);
	rdButton->text() = string("Render distance: ") +
			getName(conf->render_distance, renderDistanceOptions);
	rdButton->setOnClick([this, rdButton](){ handleClickRenderDistance(rdButton); });
	frame->add(rdButton);
	y += yIncr;
}

void Menu::handleClickFullscreen(Label *label) {
	int i = getIndex(conf->fullscreen, fullscreenOptions);
	++i;
	if (fullscreenOptions[i].desc == nullptr)
		i = 1;

	conf->fullscreen = fullscreenOptions[i].opt;
	label->text() = string("Fullscreen: ") + fullscreenOptions[i].desc;
	dirty = true;
}

void Menu::handleClickAA(Label *label) {
	int i = getIndex(aaBuf, aaOptions);
	++i;
	if (aaOptions[i].desc == nullptr)
		i = 1;

	aaBuf = aaOptions[i].opt;
	label->text() = string("Anti-aliasing: ") + aaOptions[i].desc;
}

void Menu::handleClickFog(Label *label) {
	int i = getIndex(conf->fog, fogOptions);
	++i;
	if (fogOptions[i].desc == nullptr)
		i = 1;

	conf->fog = fogOptions[i].opt;
	label->text() = string("Fog: ") + fogOptions[i].desc;
	dirty = true;
}

void Menu::handleClickRenderDistance(Label *label) {
	int i = getIndex(renderDistanceBuf, renderDistanceOptions);
	++i;
	if (renderDistanceOptions[i].desc == nullptr)
		i = 1;

	renderDistanceBuf = renderDistanceOptions[i].opt;
	label->text() = string("Render distance: ") + renderDistanceOptions[i].desc;
}

void Menu::apply() {
	if (conf->aa != aaBuf) {
		conf->aa = aaBuf;
		dirty = true;
	}

	if (conf->render_distance != renderDistanceBuf) {
		conf->render_distance = renderDistanceBuf;
		dirty = true;
	}

}
