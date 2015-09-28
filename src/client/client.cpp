#include "client.hpp"

#include <random>

#include "shared/engine/logging.hpp"
#include "shared/engine/stopwatch.hpp"
#include "shared/game/world.hpp"
#include "shared/block_manager.hpp"
#include "shared/saves.hpp"
#include "gfx/graphics.hpp"

#include "states.hpp"
#include "state_machine.hpp"
#include "states/system_init_state.hpp"
#include "states/local_playing_state.hpp"
#include "states/remote_playing_state.hpp"
#include "states/menu_state.hpp"
#include "states/connecting_state.hpp"

#include "client_chunk_manager.hpp"
#include "config.hpp"
#include "menu.hpp"
#include "events.hpp"
#include "server_interface.hpp"
#include "local_server_interface.hpp"
#include "remote_server_interface.hpp"

static logging::Logger logger("client");

int main(int argc, char *argv[]) {
	logging::init("logging.conf");

	LOG_INFO(logger) << "Starting client";
	LOG_TRACE(logger) << "Trace enabled";

	const char *worldId = "region";
	const char *serverAddress = nullptr;

	argv++;
	argc--;

	while (argc > 0) {
		if (strcmp(*argv, "-f") == 0) {
			worldId = *++argv;
			argc--;
		} else if (strcmp(*argv, "-a") == 0) {
			serverAddress = *++argv;
			argc--;
		}
		argv++;
		argc--;
	}

	initUtil();
	Client client(worldId, serverAddress);
	client.run();

	return 0;
}

Client::Client(const char *worldId, const char *serverAddress) {
	states = std::unique_ptr<States>(new States(this));
	stateMachine = std::unique_ptr<StateMachine>(new StateMachine);

	stateMachine->push(states->getSystemInit());

	if (serverAddress) {
		RemotePlayingState *playingState = states->getRemotePlaying();
		playingState->init(serverAddress);
		stateMachine->push(playingState);
	} else {
		LocalPlayingState *playingState = states->getLocalPlaying();
		playingState->init(conf->last_world_id);
		stateMachine->push(playingState);
	}

#define START_APPLICATION_IN_MENU 1
#if START_APPLICATION_IN_MENU
	stateMachine->push(states->getMenu());
#else
	graphics->grabMouse(true);
#endif

	stateMachine->push(states->getConnecting());
}

uint8 Client::getLocalClientId() const {
	 return serverInterface->getLocalClientId();
}

Character &Client::getLocalCharacter() {
	return world->getCharacter(getLocalClientId());
}

void Client::run() {
	LOG_INFO(logger) << "Running client";
	time = getCurrentTime();
	int tick = 0;
	while (!closeRequested) {
		stateMachine->update();

		stopwatch->start(CLOCK_SYN);
		sync(TICK_SPEED);
		stopwatch->stop(CLOCK_SYN);
		tick++;
	}
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
