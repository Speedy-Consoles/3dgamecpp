#include "client.hpp"

#include <iostream>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <clocale>
#include <random>

#include <boost/filesystem.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/socket.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/engine/time.hpp"
#include "shared/engine/math.hpp"
#include "shared/game/world.hpp"
#include "shared/block_manager.hpp"
#include "shared/block_utils.hpp"
#include "shared/constants.hpp"
#include "shared/saves.hpp"
#include "gui/widget.hpp"
#include "gfx/graphics.hpp"
#include "gfx/gl2/gl2_renderer.hpp"
#include "gfx/gl3/gl3_renderer.hpp"

#include "states/state.hpp"
#include "states/system_init_state.hpp"
#include "states/local_playing_state.hpp"
#include "states/remote_playing_state.hpp"

#include "config.hpp"
#include "menu.hpp"
#include "local_server_interface.hpp"
#include "remote_server_interface.hpp"
#include "events.hpp"


static logging::Logger logger("client");

int main(int argc, char *argv[]) {
	logging::init("logging.conf");

	LOG_INFO(logger) << "Starting client";
	LOG_TRACE(logger) << "Trace enabled";

	const char *worldId = "region";
	const char *serverAdress = nullptr;

	argv++;
	argc--;

	while (argc > 0) {
		if (strcmp(*argv, "-f") == 0) {
			worldId = *++argv;
			argc--;
		} else if (strcmp(*argv, "-a") == 0) {
			serverAdress = *++argv;
			argc--;
		}
		argv++;
		argc--;
	}

	initUtil();
	Client client(worldId, serverAdress);
	client.run();

	return 0;
}

Client::Client(const char *worldId, const char *serverAdress) {
	auto *system_init_state = new SystemInitState(this);
	pushState(system_init_state);

	if (serverAdress) {
		pushState(new RemotePlayingState(system_init_state, this, serverAdress));
	} else {
		pushState(new LocalPlayingState(system_init_state, this, conf->last_world_id));
	}
}

Client::~Client() {
	while (stateStack.size() > 0)
		popState();
}

uint8 Client::getLocalClientId() const {
	 return serverInterface->getLocalClientId();
}

Player &Client::getLocalPlayer() {
	return world->getPlayer(getLocalClientId());
}

void Client::pushState(State *state) {
	if (!stateStack.empty())
		stateStack.back()->hide();
	stateStack.push_back(std::unique_ptr<State>(state));
}

void Client::popState() {
	stateStack.pop_back();
	if (!stateStack.empty())
		stateStack.back()->unhide();
}

void Client::startLocalGame(std::string worldId) {
	LOG_INFO(logger) << "Opening world '" << worldId << "'";
	exitGame();
	save = std::unique_ptr<Save>(new Save(worldId));
	boost::filesystem::path path(save->getPath());
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		std::random_device rng;
		std::uniform_int_distribution<uint64> distr;
		uint64 seed = distr(rng);
		save->initialize(worldId, seed);
		save->store();
	}
	startGame();
	auto *p = new LocalServerInterface(this);
	serverInterface = std::unique_ptr<LocalServerInterface>(p);
}

void Client::startRemoteGame(std::string serverAdress) {
	LOG_INFO(logger) << "Connecting to remote server '" << serverAdress << "'";
	exitGame();
	startGame();
	auto *p = new RemoteServerInterface(this, serverAdress);
	serverInterface = std::unique_ptr<RemoteServerInterface>(p);
}

void Client::exitGame() {
	serverInterface.reset();
	renderer.reset();
	world.reset();
	chunkManager.reset();
	blockManager.reset();
	save.reset();
}

void Client::run() {
	LOG_INFO(logger) << "Running client";
	serverInterface->dispatch();
	chunkManager->dispatch();
	time = getCurrentTime();
	int tick = 0;
	while (!closeRequested) {
		Event e;
		while (e.next())
			stateStack.back()->handle(e);

		stateStack.back()->update();

#ifndef NO_GRAPHICS
        if (getCurrentTime() < time + timeShift + seconds(1) / TICK_SPEED) {
			renderer->tick();
			renderer->render();
        }
#endif

		stopwatch->start(CLOCK_SYN);
		sync(TICK_SPEED);
		stopwatch->stop(CLOCK_SYN);
		tick++;
	}
	chunkManager->wait();
	chunkManager->storeChunks();
	serverInterface->wait();
}

void Client::setConf(const GraphicsConf &newConf) {
	graphics->setConf(newConf, *conf);
	renderer->setConf(newConf, *conf);
	serverInterface->setConf(newConf, *conf);
	*conf = newConf;
}

void Client::startGame() {
	blockManager = std::unique_ptr<BlockManager>(new BlockManager());
	const char *block_ids_file = "block_ids.txt";
	if (blockManager->load(block_ids_file)) {
		LOG_ERROR(logger) << "Problem loading '" << block_ids_file << "'";
	}
	LOG_INFO(logger) << blockManager->getNumberOfBlocks() << " blocks were loaded from '" << block_ids_file << "'";

	chunkManager = std::unique_ptr<ChunkManager>(new ChunkManager(this));

	world = std::unique_ptr<World>(new World(chunkManager.get()));

	if (conf->render_backend == RenderBackend::OGL_3) {
		renderer = std::unique_ptr<GL3Renderer>(new GL3Renderer(this));
	} else {
		renderer = std::unique_ptr<GL2Renderer>(new GL2Renderer(this));
	}
}

void Client::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time + timeShift);
}
