#include "local_playing_state.hpp"

#include "client/client.hpp"

LocalPlayingState::LocalPlayingState(State *parent, Client *client, std::string worldId) :
	PlayingState(parent, client)
{
	client->startLocalGame(worldId);
	client->setStateId(Client::PLAYING);
}
