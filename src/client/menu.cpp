#include "menu.hpp"

#include "shared/block_utils.hpp"
#include "client/gfx/graphics.hpp"

#include "gui/widget.hpp"
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

Menu::Menu(Client *client) :
		_client(client)
{
	int yIncr = 20;
	int y = yIncr;

	frame = new Widget(-client->getGraphics()->getDrawWidth() / 2 + 10,
			-client->getGraphics()->getDrawHeight() / 2 + 10, 0, 0);

	auto *applyButton = new Button(0, (float) y, 100, 20);
	applyButton->text() = string("Apply");
	applyButton->setOnClick([this](){ apply(); });
	frame->add(applyButton);
	y += yIncr * 2;

	frame->add(new Label(0, (float) y, 180, 20, "Fullscreen:"));
	fsButton = new CycleButton<bool>(180, (float) y, 100, 20);
	fsButton->add(false, "Off");
	fsButton->add(true, "On");
	fsButton->setOnDataChange([this](bool b){
		this->setFullscreen(b);
	});
	frame->add(fsButton);
	y += yIncr;

	frame->add(new Label(0, (float) y, 180, 20, "Anti-aliasing:"));
	aaButton = new CycleButton<AntiAliasing>(180, (float) y, 100, 20);
	aaButton->add(AntiAliasing::NONE, "Off");
	aaButton->add(AntiAliasing::MSAA_2, "MSAA x2");
	aaButton->add(AntiAliasing::MSAA_4, "MSAA x4");
	aaButton->add(AntiAliasing::MSAA_8, "MSAA x8");
	aaButton->add(AntiAliasing::MSAA_16, "MSAA x16");
	aaButton->setOnDataChange([this](AntiAliasing aa){
		this->setAntiAliasing(aa);
	});
	frame->add(aaButton);
	y += yIncr;

	frame->add(new Label(0, (float) y, 180, 20, "Fog:"));
	fogButton = new CycleButton<Fog>(180, (float) y, 100, 20);
	fogButton->add(Fog::NONE, "Off");
	fogButton->add(Fog::FAST, "Fast");
	fogButton->add(Fog::FANCY, "Fancy");
	fogButton->setOnDataChange([this](Fog fog){
		this->setFog(fog);
	});
	frame->add(fogButton);
	y += yIncr;

	frame->add(new Label(0, (float) y, 180, 20, "Render distance:"));
	rdButton = new CycleButton<int>(180, (float) y, 100, 20);
	rdButton->add(4, "4");
	rdButton->add(8, "8");
	rdButton->add(12, "12");
	rdButton->add(16, "16");
	rdButton->add(24, "24");
	rdButton->add(32, "32");
	rdButton->add(48, "48");
	rdButton->add(64, "64");
	rdButton->setOnDataChange([this](int d){
		this->setRenderDistance(d);
	});
	frame->add(rdButton);
	y += yIncr;

	frame->add(new Label(0, (float) y, 180, 20, "Mipmapping:"));
	mipButton = new CycleButton<uint>(180, (float) y, 100, 20);
	mipButton->add(0, "Off");
	mipButton->add(1000, "Max");
	mipButton->setOnDataChange([this](uint level){
		this->setMipmapping(level);
	});
	frame->add(mipButton);
	y += yIncr;

	frame->add(new Label(0, (float) y, 180, 20, "Filtering:"));
	filtButton = new CycleButton<TexFiltering>(180, (float) y, 100, 20);
	filtButton->add(TexFiltering::NEAREST, "Nearest");
	filtButton->add(TexFiltering::LINEAR, "Linear");
	filtButton->setOnDataChange([this](TexFiltering filt){
		this->setTextureFiltering(filt);
	});
	frame->add(filtButton);
	y += yIncr;

	update();
}

void Menu::update() {
	const GraphicsConf &conf = _client->getConf();
	fsButton->set(conf.fullscreen);
	aaButton->set(conf.aa);
	fogButton->set(conf.fog);
	rdButton->set(conf.render_distance);
	mipButton->set(conf.tex_mipmapping);
	filtButton->set(conf.tex_filtering);

	bufferConf = conf;
	_dirty = false;
}


void Menu::apply() {
	if (_dirty) {
		_client->setConf(bufferConf);
		_dirty = false;
	}
}

void Menu::setFullscreen(bool b) {
	bufferConf.fullscreen = b;
	_dirty = true;
}

void Menu::setAntiAliasing(AntiAliasing aa) {
	GraphicsConf conf = _client->getConf();
	conf.aa = bufferConf.aa = aa;
	_client->setConf(conf);
}

void Menu::setFog(Fog fog) {
	GraphicsConf conf = _client->getConf();
	conf.fog = bufferConf.fog = fog;
	_client->setConf(conf);
}

void Menu::setRenderDistance(int d) {
	bufferConf.render_distance = d;
	_dirty = true;
}

void Menu::setMipmapping(uint level) {
	GraphicsConf conf = _client->getConf();
	conf.tex_mipmapping = bufferConf.tex_mipmapping = level;
	_client->setConf(conf);
}

void Menu::setTextureFiltering(TexFiltering filt) {
	GraphicsConf conf = _client->getConf();
	conf.tex_filtering = bufferConf.tex_filtering = filt;
	_client->setConf(conf);
}
