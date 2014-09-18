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
#include "menu.hpp"

using namespace std::chrono;

#include "logging.hpp"

std::ofstream cnull;

//_INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
	LOG(info) << "Starting program";

	initUtil();
	Client client;
	client.run();

	return 0;
}

Client::Client(){
	stopwatch = new Stopwatch(CLOCK_ID_NUM);
	stopwatch->start(CLOCK_ALL);

	conf = new GraphicsConf();
	load("graphics-default.profile", conf);

	world = new World();

	//serverInterface = RemoteServerInterface(args[0], world);
	serverInterface = new LocalServerInterface(world, 42, *conf);
	localClientID = serverInterface->getLocalClientID();

	menu = new Menu(conf);
	graphics = new Graphics(world, menu, localClientID, *conf, stopwatch);
}

Client::~Client() {
	store("graphics-default.profile", *conf);
	delete graphics;
	delete menu;
	delete conf;

	// world must be deleted before server interface
	delete world;
	delete serverInterface;
	delete stopwatch;
}

void Client::run() {
	LOG(info) << "Running client";
	startTimePoint = high_resolution_clock::now();
	time = 0;
	int tick = 0;
	while (!closeRequested) {
		handleInput();

		stopwatch->start(CLOCK_NET);
		serverInterface->sendInput();
		stopwatch->stop(CLOCK_NET);

		stopwatch->start(CLOCK_TIC);
		world->tick(tick, localClientID);
		stopwatch->stop(CLOCK_TIC);

		if (time + timeShift + 1000000 / TICK_SPEED > getMicroTimeSince(startTimePoint))
			graphics->tick();

		stopwatch->start(CLOCK_NET);
		serverInterface->receive(time + 200000);
		stopwatch->stop(CLOCK_NET);

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
			(int) (time + timeShift - getMicroTimeSince(startTimePoint))));
	std::this_thread::sleep_for(duration);
}

void Client::handleInput() {
	Player &player = world->getPlayer(localClientID);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case  SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				closeRequested = true;
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
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
			if (event.key.keysym.scancode == SDL_SCANCODE_O) {
				char buf[128];
				sprintf(buf, "timeShift: %ld\n", timeShift = (timeShift + 100000) % 1000000);
				LOG(info) << buf;
			} else if (event.key.keysym.scancode == SDL_SCANCODE_P) {
				char buf[128];
				sprintf(buf, "timeShift: %ld\n", timeShift = (timeShift + 900000) % 1000000);
				LOG(info) << buf;
			}
			if (!graphics->isMenu()) {
				switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					graphics->setMenu(true);
					world->setPause(true);
					break;
				case SDL_SCANCODE_F:
					serverInterface->togglePlayerFly();
					break;
				case SDL_SCANCODE_M:
					switch (conf->aa) {
					case AntiAliasing::NONE:    conf->aa = AntiAliasing::MSAA_2; break;
					case AntiAliasing::MSAA_2:  conf->aa = AntiAliasing::MSAA_4; break;
					case AntiAliasing::MSAA_4:  conf->aa = AntiAliasing::MSAA_8; break;
					case AntiAliasing::MSAA_8:  conf->aa = AntiAliasing::MSAA_16; break;
					case AntiAliasing::MSAA_16: conf->aa = AntiAliasing::NONE; break;
					}
					graphics->setConf(*conf);
					break;
//				case SDL_SCANCODE_N:
//					switch (graphics->getFXAA()) {
//					case true: graphics->disableFXAA(); break;
//					case false: graphics->enableFXAA(); break;
//					}
//					break;
				case SDL_SCANCODE_Q:
					if (SDL_GetModState() & KMOD_LCTRL)
						closeRequested = true;
					break;
				case SDL_SCANCODE_F11:
					conf->fullscreen = !conf->fullscreen;
					graphics->setConf(*conf);
					break;
				case SDL_SCANCODE_F3:
					graphics->setDebug(!graphics->isDebug());
					break;
				default:
					break;
				} // switch scancode
			} else { // if we are in menu
				switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_W: menu->navigateUp(); break;
				case SDL_SCANCODE_S: menu->navigateDown(); break;
				case SDL_SCANCODE_A: menu->navigateLeft(); break;
				case SDL_SCANCODE_D: menu->navigateRight(); break;
				case SDL_SCANCODE_ESCAPE:
					graphics->setConf(*conf);
					graphics->setMenu(false);
					serverInterface->setConf(*conf);
					world->setPause(false);
					break;
				default: break;
				}
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

	int moveInput = 0;
	if (!world->isPaused()) {
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
	}

	serverInterface->setPlayerMoveInput(moveInput);

	if (player.isValid())
		player.setMoveInput(moveInput);

	serverInterface->sendInput();
}
