#ifndef REMOTE_PLAYING_STATE_HPP_
#define REMOTE_PLAYING_STATE_HPP_

#include "playing_state.hpp"

#include <string>

class RemotePlayingState : public PlayingState {
public:
	RemotePlayingState(State *parent, Client *client, std::string address);
};

#endif // REMOTE_PLAYING_STATE_HPP_
