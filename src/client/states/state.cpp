#include "state.hpp"

#include "client/client.hpp"
#include "client/events.hpp"

void State::onPush(State *old_top) {
	parent = old_top;
	is_active = true;
	is_top = true;
}

void State::onPop() {
	parent = nullptr;
	is_active = false;
	is_top = false;
}

void State::onObscure(State *new_top) {
	is_top = false;
}

void State::onUnobscure() {
	is_top = true;
}

void State::update() {
	if (parent)
		parent->update();
}

void State::handle(const Event &e) {
	if (parent)
		parent->handle(e);
}
