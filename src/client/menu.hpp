#ifndef MENU_HPP_
#define MENU_HPP_

#include "client.hpp"
#include "config.hpp"

namespace gui {
	class Frame;
	class Label;
	template <typename T> class CycleButton;
}

#include <string>

class Menu {
	Client *_client = nullptr;
	GraphicsConf bufferConf;
	bool _dirty = false;

	gui::Frame *frame = nullptr;

	gui::CycleButton<bool> *fsButton = nullptr;
	gui::CycleButton<AntiAliasing> *aaButton = nullptr;
	gui::CycleButton<Fog> *fogButton = nullptr;
	gui::CycleButton<int> *rdButton = nullptr;
	gui::CycleButton<uint> *mipButton = nullptr;
	gui::CycleButton<TexFiltering> *filtButton = nullptr;

public:
	~Menu();
	Menu(Client *);

	gui::Frame *getFrame() { return frame; }
	const gui::Frame *getFrame() const { return frame; }

	void update();
	void apply();

private:
	void setFullscreen(bool);
	void setAntiAliasing(AntiAliasing);
	void setFog(Fog);
	void setRenderDistance(int);
	void setMipmapping(uint);
	void setTextureFiltering(TexFiltering);
};

#endif // MENU_HPP_
