#ifndef TEXT_INPUT_STATE_HPP_
#define TEXT_INPUT_STATE_HPP_

#include "state.hpp"

#include <string>

class Client;

class TextInputState : public State {
public:
	TextInputState(State *parent, Client *client, std::string *target);
	~TextInputState();

	void handle(const Event &) override;

private:
	std::string *target = nullptr;
};

#endif // TEXT_INPUT_STATE_HPP_
