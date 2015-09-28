#include "local_playing_state.hpp"

#include <boost/filesystem.hpp>

#include "shared/saves.hpp"
#include "shared/engine/random.hpp"

#include "client/client.hpp"
#include "client/client_chunk_manager.hpp"
#include "client/local_server_interface.hpp"

void LocalPlayingState::init(std::string world_id) {
	_world_id = world_id;
}

void LocalPlayingState::onPush(State *old_top) {
	PlayingState::onPush(old_top);

	client->save = std::unique_ptr<Save>(new Save(_world_id));
	boost::filesystem::path path(client->save->getPath());
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		std::random_device rng;
		uniform_int_distribution<uint64> distr;
		uint64 seed = distr(rng);
		client->save->initialize(_world_id, seed);
		client->save->store();
	}

	ClientChunkManager *cm = new ClientChunkManager(client, client->save->getChunkArchive());
	client->chunkManager = std::unique_ptr<ClientChunkManager>(cm);
	client->world = std::unique_ptr<World>(new World(client->chunkManager.get()));

	LocalServerInterface *si = new LocalServerInterface(client);
	client->serverInterface = std::unique_ptr<LocalServerInterface>(si);

	client->setStateId(Client::PLAYING);
}
