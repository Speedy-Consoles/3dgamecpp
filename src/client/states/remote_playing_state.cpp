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

	RemoteServerInterface *rsi = client->remoteServerInterface.get();
	if (!rsi) {
		rsi = new RemoteServerInterface(client);
		client->remoteServerInterface = std::unique_ptr<RemoteServerInterface>(rsi);
	}
	client->serverInterface = rsi;
	rsi->connect(address);

	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));
}

RemotePlayingState::~RemotePlayingState() {
	client->remoteServerInterface.get()->waitForDisconnect();
}
