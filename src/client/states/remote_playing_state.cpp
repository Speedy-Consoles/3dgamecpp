#include "remote_playing_state.hpp"

#include "client/client.hpp"
#include "client/client_chunk_manager.hpp"
#include "client/remote_server_interface.hpp"
#include "client/states/connecting_state.hpp"

RemotePlayingState::RemotePlayingState(State *parent, Client *client, std::string address) :
	PlayingState(parent, client)
{
	ChunkArchive *ca = new ChunkArchive("chunk_cache/dummy/");
	ClientChunkManager *cm = new ClientChunkManager(client, std::unique_ptr<ChunkArchive>(ca));
	client->chunkManager = std::unique_ptr<ClientChunkManager>(cm);

	RemoteServerInterface *si = new RemoteServerInterface(client, address);
	client->serverInterface = std::unique_ptr<RemoteServerInterface>(si);

	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));
}
