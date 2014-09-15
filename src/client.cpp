#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "util.hpp"
#include "constants.hpp"
#include "client.hpp"
#include "local_server_interface.hpp"
#include "graphics.hpp"
#include "config.hpp"

using namespace std::chrono;

#include "logging.hpp"

_INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
	const char *el_argv[] = {
//		"--v=2",
	};
	size_t el_argc = sizeof (el_argv) / sizeof (const char *);
	_START_EASYLOGGINGPP(el_argc, el_argv);
	el::Configurations loggingConf;
	loggingConf.setGlobally(el::ConfigurationType::Format,
			"%datetime{%Y-%M-%d %h:%m:%s,%g} %levshort: %msg");
	loggingConf.set(el::Level::Error, el::ConfigurationType::Format,
			"%datetime{%Y-%M-%d %h:%m:%s,%g} %levshort (%loc): %msg");
	loggingConf.set(el::Level::Warning, el::ConfigurationType::Format,
			"%datetime{%Y-%M-%d %h:%m:%s,%g} %levshort (%loc): %msg");
	el::Loggers::reconfigureLogger("default", loggingConf);

	LOG(INFO) << "Starting program";

	initUtil();
	Client client;
	client.run();

	return 0;
}

Client::Client() : stopwatch(nullptr) {
	stopwatch = new Stopwatch(CLOCK_ID_NUM);
	stopwatch->start(CLOCK_ALL);

	world = new World();

	//serverInterface = RemoteServerInterface(args[0], world);
	serverInterface = new LocalServerInterface(world, 42);
	localClientID = serverInterface->getLocalClientID();

	GraphicsConf conf;
	load("graphics-default.profile", &conf);
	graphics = new Graphics(world, localClientID, conf, stopwatch);
}

Client::~Client() {
	auto conf = graphics->getConf();
	store("graphics-default.profile", conf);
	delete graphics;

	// world must be deleted before server interface
	delete world;
	delete serverInterface;
	delete stopwatch;
}

void Client::run() {
	LOG(INFO) << "Running client";
	startTimePoint = high_resolution_clock::now();
	time = 0;
	int tick = 0;
	while (!closeRequested) {
		handleInput();

		stopwatch->start(CLOCK_NET);
		serverInterface->sendInput();
		serverInterface->receive(time + 200000);
		stopwatch->stop(CLOCK_NET);

		stopwatch->start(CLOCK_TIC);
		world->tick(tick, localClientID);
		stopwatch->stop(CLOCK_TIC);

		if (time + 1000000 / TICK_SPEED > getMicroTimeSince(startTimePoint)) {
			graphics->tick();
		}

		stopwatch->start(CLOCK_SYN);
		sync(TICK_SPEED);
		stopwatch->stop(CLOCK_SYN);
		tick++;
	}
	serverInterface->stop();
}

void Client::sync(int perSecond) {
	time = time + 1000000 / perSecond;
	microseconds duration(std::max(0,
			(int) (time - getMicroTimeSince(startTimePoint))));
	std::this_thread::sleep_for(duration);
}

void Client::handleInput() {
	int moveInput = 0;

	Player &player = world->getPlayer(localClientID);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case  SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				closeRequested = true;
				break;
			case SDL_WINDOWEVENT_RESIZED:
				graphics->resize(
					event.window.data1,
					event.window.data2
				);
				break;
			default:
				break;
			}
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_ESCAPE:
				graphics->setMenu(!graphics->isMenu());
				world->setPause(!world->isPaused());
				break;
			case SDL_SCANCODE_F:
				serverInterface->togglePlayerFly();
				break;
			case SDL_SCANCODE_M:
				switch (graphics->getMSAA()) {
				case 0: graphics->enableMSAA(2); break;
				case 2: graphics->enableMSAA(4); break;
				case 4: graphics->enableMSAA(8); break;
				case 8: graphics->enableMSAA(16); break;
				case 16: graphics->disableMSAA(); break;
				}
				break;
//			case SDL_SCANCODE_N:
//				switch (graphics->getFXAA()) {
//				case true: graphics->disableFXAA(); break;
//				case false: graphics->enableFXAA(); break;
//				}
//				break;
			case SDL_SCANCODE_Q:
				if (SDL_GetModState() & KMOD_LCTRL)
					closeRequested = true;
				break;
			default:
				//printf("unknown key: ", event.key.keysym);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			if (!graphics->isMenu() && player.isValid()) {
				double yaw = player.getYaw();
				double pitch = player.getPitch();
				yaw -= event.motion.xrel / 10.0;
				pitch -= event.motion.yrel / 10.0;
				yaw = fmod(yaw, 360);
				pitch = std::max(pitch, -90.0);
				pitch = std::min(pitch, 90.0);
				serverInterface->setPlayerOrientation(yaw, pitch);
				player.setOrientation(yaw, pitch);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			{
				using namespace vec_auto_cast;
				vec3i64 bc;
				int d;
				bool target = player.getTargetedFace(&bc, &d);
				if (target) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						vec3i64 rbc = bc + DIRS[d];
						serverInterface->edit(rbc, 1);
					} else if (event.button.button == SDL_BUTTON_RIGHT) {
						serverInterface->edit(bc, 0);
					}
				}
			}
		}
	}

	const uint8 *keyboard = SDL_GetKeyboardState(nullptr);

	if (keyboard[SDL_SCANCODE_D])
		moveInput |= Player::MOVE_INPUT_FLAG_STRAFE_RIGHT;
	if (keyboard[SDL_SCANCODE_A])
		moveInput |= Player::MOVE_INPUT_FLAG_STRAFE_LEFT;

	if (keyboard[SDL_SCANCODE_SPACE])
		moveInput |= Player::MOVE_INPUT_FLAG_FLY_UP;
	if (keyboard[SDL_SCANCODE_LCTRL])
		moveInput |= Player::MOVE_INPUT_FLAG_FLY_DOWN;

	if (keyboard[SDL_SCANCODE_W])
		moveInput |= Player::MOVE_INPUT_FLAG_MOVE_FORWARD;
	if (keyboard[SDL_SCANCODE_S])
		moveInput |= Player::MOVE_INPUT_FLAG_MOVE_BACKWARD;

	if (keyboard[SDL_SCANCODE_LSHIFT])
		moveInput |= Player::MOVE_INPUT_FLAG_SPRINT;

	serverInterface->setPlayerMoveInput(moveInput);

	if (player.isValid())
		player.setMoveInput(moveInput);

	serverInterface->sendInput();
}
