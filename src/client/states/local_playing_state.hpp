#ifndef LOCAL_PLAYING_STATE_HPP_
#define LOCAL_PLAYING_STATE_HPP_

#include "playing_state.hpp"

#include <string>

class Client;

class LocalPlayingState : public PlayingState {
public:
	LocalPlayingState(Client *client) : PlayingState(client) {};

	void init(std::string world_id);

	void onPush(State *) override;

private:
	std::string _world_id;
};

#endif // LOCAL_PLAYING_STATE_HPP_
