#include "text_input_state.hpp"

#include <SDL2/SDL_keyboard.h>

#include "client/client.hpp"
#include "client/events.hpp"
#include "shared/engine/logging.hpp"

static logging::Logger logger("state");

TextInputState::TextInputState(State *parent, Client *client, std::string *target) :
	State(parent, client),
	target(target)
{
	LOG_DEBUG(logger) << "Entering TextInputState";
	SDL_StartTextInput();
}

TextInputState::~TextInputState() {
	LOG_DEBUG(logger) << "Exiting TextInputState";
	SDL_StopTextInput();
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
		case SDL_SCANCODE_ESCAPE: {
			// save parent, because client->popState will actually destroy 'this'
			auto parent_copy = parent;
			client->popState();
			parent_copy->handle(e);
			break;
		}
		case SDL_SCANCODE_RETURN:
			client->popState();
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
