#include "local_playing_state.hpp"

#include <boost/filesystem.hpp>

#include "shared/saves.hpp"
#include "shared/engine/random.hpp"

#include "client/client.hpp"
#include "client/local_server_interface.hpp"

LocalPlayingState::LocalPlayingState(State *parent, Client *client, std::string worldId) :
	PlayingState(parent, client)
{
	client->save = std::unique_ptr<Save>(new Save(worldId));
	boost::filesystem::path path(client->save->getPath());
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		std::random_device rng;
		uniform_int_distribution<uint64> distr;
		uint64 seed = distr(rng);
		client->save->initialize(worldId, seed);
		client->save->store();
	}

	client->chunkManager = std::unique_ptr<ChunkManager>(new ChunkManager(client));
	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));
	auto *p = new LocalServerInterface(client);
	client->serverInterface = std::unique_ptr<LocalServerInterface>(p);
	client->setStateId(Client::PLAYING);
}
