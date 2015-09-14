#ifndef LOCAL_PLAYING_STATE_HPP_
#define LOCAL_PLAYING_STATE_HPP_

#include "playing_state.hpp"

#include <string>

class Client;

class LocalPlayingState : public PlayingState {
public:
	LocalPlayingState(State *parent, Client *client, std::string worldId);
};

#endif // LOCAL_PLAYING_STATE_HPP_
