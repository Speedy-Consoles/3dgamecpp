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
#include "gui/cycle_button.hpp"

using namespace std;
using namespace gui;

template class CycleButton<bool>;
template class CycleButton<int>;
template class CycleButton<Fog>;
template class CycleButton<AntiAliasing>;

Menu::~Menu() {
	delete frame;
}

Menu::Menu(GraphicsConf *conf) :
	conf(conf)
{
	renderDistanceBuf = conf->render_distance;
	aaBuf = conf->aa;

	int yIncr = 20;
	int y = yIncr;

	frame = new Frame(10, 10, 0, 0);

	auto *applyButton = new Button(0, y, 100, 20);
	applyButton->text() = string("Apply");
	applyButton->setOnClick([this](){ apply(); });
	frame->add(applyButton);
	y += yIncr * 2;

	frame->add(new Label(0, y, 180, 20, "Fullscreen:"));
	fsButton = new CycleButton<bool>(180, y, 100, 20);
	fsButton->add(false, "Off");
	fsButton->add(true, "On");
	fsButton->setOnDataChange([this](bool b){
		this->conf->fullscreen = b;
		this->dirty = true;
	});
	frame->add(fsButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Anti-aliasing:"));
	aaButton = new CycleButton<AntiAliasing>(180, y, 100, 20);
	aaButton->add(AntiAliasing::NONE, "Off");
	aaButton->add(AntiAliasing::MSAA_2, "MSAA x2");
	aaButton->add(AntiAliasing::MSAA_4, "MSAA x4");
	aaButton->add(AntiAliasing::MSAA_8, "MSAA x8");
	aaButton->add(AntiAliasing::MSAA_16, "MSAA x16");
	aaButton->setOnDataChange([this](AntiAliasing aa){
		this->aaBuf = aa;
	});
	frame->add(aaButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Fog:"));
	fogButton = new CycleButton<Fog>(180, y, 100, 20);
	fogButton->add(Fog::NONE, "Off");
	fogButton->add(Fog::FAST, "Fast");
	fogButton->add(Fog::FANCY, "Fancy");
	fogButton->setOnDataChange([this](Fog fog){
		this->conf->fog = fog;
		this->dirty = true;
	});
	frame->add(fogButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Render distance:"));
	rdButton = new CycleButton<int>(180, y, 100, 20);
	rdButton->add(4, "4");
	rdButton->add(8, "8");
	rdButton->add(12, "12");
	rdButton->add(16, "16");
	rdButton->add(24, "24");
	rdButton->add(32, "32");
	rdButton->setOnDataChange([this](int rd){
		this->renderDistanceBuf = rd;
	});
	frame->add(rdButton);
	y += yIncr;

	update();
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

void Menu::update() {
	fsButton->set(conf->fullscreen);
	aaButton->set(conf->aa);
	fogButton->set(conf->fog);
	rdButton->set(conf->render_distance);

	aaBuf = conf->aa;
	renderDistanceBuf = conf->render_distance;
}
