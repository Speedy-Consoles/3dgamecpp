#ifndef CONNECTING_STATE_HPP
#define CONNECTING_STATE_HPP

#include "state.hpp"

class Client;

class ConnectingState : public State {
public:
	ConnectingState(Client *client) : State(client) {};

	void onPush(State *) override;

	void update() override;
};

#endif // CONNECTING_STATE_HPP
