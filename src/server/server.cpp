#include <SDL2/SDL_net.h>
#include <chrono>
#include <thread>

#include "std_types.hpp"
#include "io/logging.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "game/world.hpp"

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("server")

class Server {
private:
	World *world;
	int64 time = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
	bool closeRequested = false;
public:
	Server(uint16 port);
	~Server();
	void run();
	void sync(int perSecond);
};

int main(int argc, char *argv[]) {
	initLogging("logging_srv.conf");

	LOG(INFO, "Starting server");
	LOG(TRACE, "Trace enabled");

	initUtil();
	Server server(8547);
	server.run();

	return 0;
}

Server::Server(uint16 port) {
	world = new World();
	printf("%d\n", port);
}

Server::~Server() {

}

void Server::run() {
	LOG(INFO, "Running server");
	startTimePoint = high_resolution_clock::now();
	time = 0;
	int tick = 0;
	while (!closeRequested) {
		world->tick(tick, -1);
		sync(TICK_SPEED);
		tick++;
	}
}

void Server::sync(int perSecond) {
	time = time + 1000000 / perSecond;
	microseconds duration(std::max(0,
			(int) (time - getMicroTimeSince(startTimePoint))));
	std::this_thread::sleep_for(duration);
}
