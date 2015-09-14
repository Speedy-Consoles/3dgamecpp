#ifndef SYSTEM_INIT_STATE_HPP
#define SYSTEM_INIT_STATE_HPP

#include "state.hpp"

class Client;

class SystemInitState : public State {
public:
	SystemInitState(Client *client);
	~SystemInitState();

	void handle(const Event &) override;
};

#endif // SYSTEM_INIT_STATE_HPP
