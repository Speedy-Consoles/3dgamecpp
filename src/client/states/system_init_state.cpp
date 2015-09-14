#include "system_init_state.hpp"

#include "client/client.hpp"
#include "client/config.hpp"
#include "client/events.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gui/widget.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/engine/logging.hpp"

static logging::Logger logger("client");

SystemInitState::SystemInitState(Client *client) :
	State(nullptr, client)
{
	client->stopwatch = std::unique_ptr<Stopwatch>(new Stopwatch(CLOCK_ID_NUM));
	client->stopwatch->start(CLOCK_ALL);

	client->conf = std::unique_ptr<GraphicsConf>(new GraphicsConf());
	load("graphics-default.profile", client->conf.get());

	client->graphics = std::unique_ptr<Graphics>(new Graphics(client));
	client->graphics->createContext();

	client->menu = std::unique_ptr<Menu>(new Menu(client));
}

SystemInitState::~SystemInitState() {
	store("graphics-default.profile", *client->conf);
}

void SystemInitState::handle(const Event &e) {
	switch (e.type) {

	case EventType::WINDOW_CLOSE:
		client->closeRequested = true;
		break;

	case EventType::WINDOW_RESIZED:
		client->graphics->resize(
			e.event.window.data1,
			e.event.window.data2
		);
		client->renderer->resize();
		client->menu->getFrame()->move(
			-client->graphics->getDrawWidth() / 2 + 10,
			-client->graphics->getDrawHeight() / 2 + 10
		);
		break;

	case EventType::KEYBOARD_PRESSED:
		switch (e.event.key.keysym.scancode) {
		case SDL_SCANCODE_Q:
			if (SDL_GetModState() & KMOD_LCTRL)
				client->closeRequested = true;
			break;

		case SDL_SCANCODE_O:
			client->timeShift = (client->timeShift + millis(100)) % millis(1000);
			break;

		case SDL_SCANCODE_P:
			client->timeShift = (client->timeShift + millis(900)) % millis(1000);
			break;

		default:
			break;
		} // switch scancode
		break;

	default:
		break;

	} // switch event type
}