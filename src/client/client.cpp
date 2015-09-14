#include "client.hpp"

#include <random>

#include "shared/engine/logging.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/game/world.hpp"
#include "shared/block_manager.hpp"
#include "shared/saves.hpp"
#include "gfx/graphics.hpp"

#include "states/state.hpp"
#include "states/system_init_state.hpp"
#include "states/local_playing_state.hpp"
#include "states/remote_playing_state.hpp"

#include "config.hpp"
#include "menu.hpp"
#include "events.hpp"
#include "server_interface.hpp"

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

void Client::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time + timeShift);
}
