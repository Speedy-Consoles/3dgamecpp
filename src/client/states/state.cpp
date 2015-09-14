#include "state.hpp"

#include "client/client.hpp"
#include "client/events.hpp"

void State::handle(const Event &e) {
	if (parent)
		parent->handle(e);
}
