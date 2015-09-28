#ifndef PLAYING_STATE_HPP
#define PLAYING_STATE_HPP

#include "state.hpp"

class Client;
class ServerInterface;

class PlayingState : public State {
public:
	PlayingState(Client *client) : State(client) {};
	
	void onPush(State *) override;
	void onPop() override;

	void onUnobscure() override;

	void update() override;
	void handle(const Event &) override;
};

#endif // PLAYING_STATE_HPP
