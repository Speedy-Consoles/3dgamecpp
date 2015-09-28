#ifndef TEXT_INPUT_STATE_HPP_
#define TEXT_INPUT_STATE_HPP_

#include "state.hpp"

#include <string>

class Client;

class TextInputState : public State {
public:
	TextInputState(Client *client) : State(client) {}

	void init(std::string *target);

	void onPush(State *) override;
	void onPop() override;

	void handle(const Event &) override;

private:
	std::string *target;
};

#endif // TEXT_INPUT_STATE_HPP_
