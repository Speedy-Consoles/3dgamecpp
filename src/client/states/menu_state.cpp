#include "menu_state.hpp"

#include "client/client.hpp"
#include "client/events.hpp"
#include "client/config.hpp"
#include "client/menu.hpp"
#include "client/state_machine.hpp"
#include "client/sounds.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gui/widget.hpp"

void MenuState::onPush(State *old_top) {
	State::onPush(old_top);
	menu = client->getMenu();
	menu->update();
	client->getGraphics()->grabMouse(false);
	client->setStateId(Client::MENU);
}

void MenuState::onPop() {
	menu->apply();
	State::onPop();
}

void MenuState::onUnobscure() {
	State::onUnobscure();
	client->setStateId(Client::MENU);
}

void MenuState::handle(const Event &e) {
	Graphics *graphics = client->getGraphics();

	switch (e.type) {

	case EventType::MOUSE_MOTION: {
		int x = e.event.motion.x - graphics->getWidth() / 2;
		int y = graphics->getHeight() / 2 - e.event.motion.y;
		float factor = graphics->getScalingFactor();
		menu->getFrame()->updateMousePosition(x * factor, y * factor);
		break;
	} // case MOUSE_MOTION

	case EventType::MOUSE_BUTTON_PRESSED: {
		int x = e.event.button.x - graphics->getWidth() / 2;
		int y = graphics->getHeight() / 2 - e.event.button.y;
		float factor = graphics->getScalingFactor();
		menu->getFrame()->handleMouseClick(x * factor, y * factor);
		break;
	} // case MOUSE_BUTTON_PRESSED

	case EventType::KEYBOARD_PRESSED: {
		switch (e.event.key.keysym.scancode) {
		case SDL_SCANCODE_ESCAPE:
			client->getSounds()->play("menu_close");
			client->getStateMachine()->pop();
			break;
		default:
			parent->handle(e);
			break;
		} // switch scancode
		break;
	} // case KEYBOARD_PRESSED

	default:
		parent->handle(e);
		break;

	} // switch event type
}