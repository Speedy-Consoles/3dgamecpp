#include "remote_playing_state.hpp"

#include "client/client.hpp"
#include "client/remote_server_interface.hpp"
#include "client/states/connecting_state.hpp"

RemotePlayingState::RemotePlayingState(State *parent, Client *client, std::string adress) :
	PlayingState(parent, client)
{
	client->chunkManager = std::unique_ptr<ChunkManager>(new ChunkManager(client));
	auto *p = new RemoteServerInterface(client, adress);
	client->serverInterface = std::unique_ptr<RemoteServerInterface>(p);
	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));
	client->pushState(new ConnectingState(this, client));
}
