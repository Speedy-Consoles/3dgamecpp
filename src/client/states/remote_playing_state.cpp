#include "remote_playing_state.hpp"

#include "client/client.hpp"
#include "client/client_chunk_manager.hpp"
#include "client/remote_server_interface.hpp"
#include "client/states/connecting_state.hpp"

RemotePlayingState::RemotePlayingState(State *parent, Client *client, std::string adress) :
	PlayingState(parent, client)
{
	ChunkArchive *ca = new ChunkArchive("chunk_cache/dummy/");
	ClientChunkManager *cm = new ClientChunkManager(client, std::unique_ptr<ChunkArchive>(ca));
	client->chunkManager = std::unique_ptr<ClientChunkManager>(cm);
	auto *p = new RemoteServerInterface(client, adress);
	client->serverInterface = std::unique_ptr<RemoteServerInterface>(p);
	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));
}
