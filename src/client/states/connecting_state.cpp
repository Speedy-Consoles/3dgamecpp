#include "connecting_state.hpp"

#include "client/client.hpp"
#include "client/state_machine.hpp"
#include "client/server_interface.hpp"

void ConnectingState::onPush(State *old_top) {
	State::onPush(old_top);
	client->setStateId(Client::CONNECTING);
}

void ConnectingState::update() {
	// parent->update();

	client->getServerInterface()->tick();
	if (client->getServerInterface()->getStatus() == ServerInterface::CONNECTED) {
		client->getStateMachine()->pop();
	}
}