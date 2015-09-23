#include "connecting_state.hpp"

#include "client/client.hpp"
#include "client/server_interface.hpp"

ConnectingState::ConnectingState(State *parent, Client *client) :
	State(parent, client)
{
	client->setStateId(Client::CONNECTING);
}

void ConnectingState::update() {
	// parent->update();

	client->getServerInterface()->tick();
	if (client->getServerInterface()->getStatus() == ServerInterface::CONNECTED) {
		client->popState();
	}
}