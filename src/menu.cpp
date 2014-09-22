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

Menu::Menu(GraphicsConf *c) :
		clientConf(c), bufferConf(*c)
{
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
		this->clientConf->fullscreen = b;
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
		this->bufferConf.aa = aa;
	});
	frame->add(aaButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Fog:"));
	fogButton = new CycleButton<Fog>(180, y, 100, 20);
	fogButton->add(Fog::NONE, "Off");
	fogButton->add(Fog::FAST, "Fast");
	fogButton->add(Fog::FANCY, "Fancy");
	fogButton->setOnDataChange([this](Fog fog){
		this->clientConf->fog = fog;
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
		this->bufferConf.render_distance = rd;
	});
	frame->add(rdButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Mipmapping:"));
	mipButton = new CycleButton<uint>(180, y, 100, 20);
	mipButton->add(0, "Off");
//	mipButton->add(1, "1");
//	mipButton->add(2, "2");
//	mipButton->add(3, "3");
//	mipButton->add(4, "4");
//	mipButton->add(5, "5");
//	mipButton->add(6, "6");
//	mipButton->add(7, "7");
//	mipButton->add(8, "8");
//	mipButton->add(9, "9");
	mipButton->add(1000, "Max");
	mipButton->setOnDataChange([this](uint mip){
		this->bufferConf.tex_mipmapping = mip;
	});
	frame->add(mipButton);
	y += yIncr;

	frame->add(new Label(0, y, 180, 20, "Filtering:"));
	filtButton = new CycleButton<TexFiltering>(180, y, 100, 20);
	filtButton->add(TexFiltering::NEAREST, "Nearest");
	filtButton->add(TexFiltering::LINEAR, "Linear");
	filtButton->setOnDataChange([this](TexFiltering filt){
		this->bufferConf.tex_filtering = filt;
	});
	frame->add(filtButton);
	y += yIncr;

	update();
}

void Menu::apply() {
	*clientConf = bufferConf;
	dirty = true;
}

void Menu::update() {
	fsButton->set(clientConf->fullscreen);
	aaButton->set(clientConf->aa);
	fogButton->set(clientConf->fog);
	rdButton->set(clientConf->render_distance);
	mipButton->set(clientConf->tex_mipmapping);
	filtButton->set(clientConf->tex_filtering);

	bufferConf = *clientConf;
}
