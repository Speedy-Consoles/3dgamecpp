#include "remote_playing_state.hpp"

#include "client/client.hpp"
#include "client/states/connecting_state.hpp"

RemotePlayingState::RemotePlayingState(State *parent, Client *client, std::string adress) :
	PlayingState(parent, client)
{
	client->startRemoteGame(adress);
	client->pushState(new ConnectingState(this, client));
}
