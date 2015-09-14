#ifndef CONNECTING_STATE_HPP
#define CONNECTING_STATE_HPP

#include "state.hpp"

class Client;

class ConnectingState : public State {
public:
	ConnectingState(State *parent, Client *client);

	void update() override;
};

#endif // CONNECTING_STATE_HPP
