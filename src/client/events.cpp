#include "events.hpp"

bool Event::next() {
	bool hasEvent = SDL_PollEvent(&this->event) != 0;
	if (hasEvent) {
		switch (event.type) {
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SHOWN:        type = EventType::WINDOW_SHOWN; break;
			case SDL_WINDOWEVENT_HIDDEN:       type = EventType::WINDOW_HIDDEN; break;
			case SDL_WINDOWEVENT_EXPOSED:      type = EventType::WINDOW_EXPOSED; break;
			case SDL_WINDOWEVENT_MOVED:        type = EventType::WINDOW_MOVED; break;
			case SDL_WINDOWEVENT_RESIZED:      type = EventType::WINDOW_RESIZED; break;
			case SDL_WINDOWEVENT_SIZE_CHANGED: type = EventType::WINDOW_SIZE_CHANGED; break;
			case SDL_WINDOWEVENT_MINIMIZED:    type = EventType::WINDOW_MINIMIZED; break;
			case SDL_WINDOWEVENT_MAXIMIZED:    type = EventType::WINDOW_MAXIMIZED; break;
			case SDL_WINDOWEVENT_RESTORED:     type = EventType::WINDOW_RESTORED; break;
			case SDL_WINDOWEVENT_ENTER:        type = EventType::WINDOW_ENTER; break;
			case SDL_WINDOWEVENT_LEAVE:        type = EventType::WINDOW_LEAVE; break;
			case SDL_WINDOWEVENT_FOCUS_GAINED: type = EventType::WINDOW_FOCUS_GAINED; break;
			case SDL_WINDOWEVENT_FOCUS_LOST:   type = EventType::WINDOW_FOCUS_LOST; break;
			case SDL_WINDOWEVENT_CLOSE:        type = EventType::WINDOW_CLOSE; break;
			default:                           type = EventType::OTHER; break;
			}
			break;
		case SDL_KEYDOWN:
			type = event.key.repeat ? EventType::KEYBOARD_REPEAT : EventType::KEYBOARD_PRESSED;
			break;
		case SDL_KEYUP:
			type = EventType::KEYBOARD_RELEASED;
			break;
		case SDL_MOUSEMOTION:
			type = EventType::MOUSE_MOTION;
			break;
		case SDL_MOUSEBUTTONDOWN:
			type = EventType::MOUSE_BUTTON_PRESSED;
			break;
		case SDL_MOUSEBUTTONUP:
			type = EventType::MOUSE_BUTTON_RELEASED;
			break;
		case SDL_MOUSEWHEEL:
			type = EventType::MOUSE_WHEEL;
			break;
		default:
			type = EventType::OTHER;
			break;
		}
	} else {
		type = EventType::NONE;
	}
	return hasEvent;
}