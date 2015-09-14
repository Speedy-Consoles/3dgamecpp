#ifndef PLAYING_STATE_HPP
#define PLAYING_STATE_HPP

#include "state.hpp"

class Client;
class ServerInterface;

class PlayingState : public State {
public:
	PlayingState(State *state, Client *client);
	~PlayingState();
	
	void hide() override;
	void unhide() override;

	void update() override;
	void handle(const Event &) override;

private:
	bool hidden = false;
};

#endif // PLAYING_STATE_HPP
