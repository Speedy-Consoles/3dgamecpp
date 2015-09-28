#include "text_input_state.hpp"

#include <SDL2/SDL_keyboard.h>

#include "client/client.hpp"
#include "client/events.hpp"
#include "client/state_machine.hpp"
#include "shared/engine/logging.hpp"

static logging::Logger logger("state");

void TextInputState::init(std::string *target) {
	this->target = target;
}

void TextInputState::onPush(State *old_top) {
	State::onPush(old_top);
	LOG_DEBUG(logger) << "Entering TextInputState";
	SDL_StartTextInput();
}

void TextInputState::onPop() {
	LOG_DEBUG(logger) << "Exiting TextInputState";
	SDL_StopTextInput();
	State::onPop();
}

void TextInputState::handle(const Event &e) {
	switch (e.type) {

	case EventType::TEXT_INPUT: {
		target->append(e.event.text.text);
		break;
	} // case TEXT_INPUT

	case EventType::TEXT_EDIT: {
		LOG_TRACE(logger) << "TextInputState received '" << e.event.text.text << "' edit";
		break;
	} // case TEXT_EDIT
							   
	case EventType::KEYBOARD_PRESSED: 
	case EventType::KEYBOARD_REPEAT:
	{
		switch (e.event.key.keysym.scancode) {
		case SDL_SCANCODE_ESCAPE:
		case SDL_SCANCODE_RETURN:
			client->getStateMachine()->pop();
			break;
		case SDL_SCANCODE_BACKSPACE:
			byte octet;
			if (target->size() > 0) {
				// take advantage of utf-8 properties
				octet = (byte)target->back();
				if ((octet & 0x80) == 0x00) {
					target->pop_back();
				} else {
					while (target->size() > 0 && (octet & 0xC0) == 0x80) {
						target->pop_back();
						octet = (byte)target->back();
					}
					if (target->size() > 0 && (octet & 0xC0) == 0xC0) {
						target->pop_back();
					} else {
						LOG_WARNING(logger) << "Invalid UTF-8 string in TextInputState";
					}
				}
			}
			
			break;
		default:
			break;
		} // switch scancode
		break;
	} // case KEYBOARD_PRESSED, KEYBOARD_REPEAT

	default:
		parent->handle(e);
		break;

	} // switch event type
}
