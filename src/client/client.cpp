#include "client.hpp"

#include <iostream>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "util.hpp"
#include "constants.hpp"
#include "local_server_interface.hpp"
#include "remote_server_interface.hpp"
#include "graphics.hpp"
#include "config.hpp"
#include "stopwatch.hpp"
#include "time.hpp"
#include "game/world.hpp"
#include "menu.hpp"
#include "gui/frame.hpp"
#include "io/logging.hpp"
#include "net/socket.hpp"

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("client")

class Client {
private:
	ServerInterface *serverInterface = nullptr;
	World *world = nullptr;
	Menu *menu = nullptr;
	gui::Frame *frame = nullptr;
	Graphics *graphics = nullptr;
	GraphicsConf *conf = nullptr;
	Stopwatch *stopwatch = nullptr;

	uint8 localClientId;

	ClientState state = CONNECTING;

	Time time = 0;
    Time timeShift = 0;

	bool closeRequested = false;

public:
	Client(const Client &) = delete;
	Client(const char *worldId, const char *serverAdress);
	~Client();

	void run();

private:
	void sync(int perSecond);

	void handleInput();
};

int main(int argc, char *argv[]) {
	initLogging("logging.conf");

	LOG(INFO, "Starting client");
	LOG(TRACE, "Trace enabled");

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
	stopwatch = new Stopwatch(CLOCK_ID_NUM);
	stopwatch->start(CLOCK_ALL);

	conf = new GraphicsConf();
	load("graphics-default.profile", conf);

	LOG(INFO, "Opening world '" << worldId << "'");
	world = new World(worldId);

	menu = new Menu(conf);
	frame = menu->getFrame();
	graphics = new Graphics(world, menu, &state, &localClientId, *conf, stopwatch);

	if (serverAdress) {
		LOG(INFO, "Connecting to remote server '" << serverAdress << "'");
		serverInterface = new RemoteServerInterface(world, serverAdress, *conf);
	} else {
		LOG(INFO, "Connecting to local server");
		serverInterface = new LocalServerInterface(world, 42, *conf);
	}
}

Client::~Client() {
	store("graphics-default.profile", *conf);
	delete graphics;
	delete menu;

	// world must be deleted before server interface
	delete world;
	delete serverInterface;
	delete stopwatch;

	delete conf;
}

void Client::run() {
	LOG(INFO, "Running client");
	time = getCurrentTime();
	int tick = 0;
	while (!closeRequested) {
		handleInput();

		if (state == CONNECTING && serverInterface->getStatus() == ServerInterface::CONNECTED) {
			state = PLAYING;
			localClientId = serverInterface->getLocalClientId();
		}

		if (state == PLAYING) {
			stopwatch->start(CLOCK_NET);
			serverInterface->sendInput();
			stopwatch->stop(CLOCK_NET);
		}
		if (state == PLAYING || state == IN_MENU) {
			stopwatch->start(CLOCK_NET);
			serverInterface->receive(time + 200000);
			stopwatch->stop(CLOCK_NET);

			stopwatch->start(CLOCK_TIC);
			world->tick(tick, localClientId);
			stopwatch->stop(CLOCK_TIC);
		}
#ifndef NO_GRAPHICS
		if (getCurrentTime() < time + timeShift + seconds(1) / TICK_SPEED)
			graphics->tick();

#endif

		// TODO finish here

		stopwatch->start(CLOCK_SYN);
		sync(TICK_SPEED);
		stopwatch->stop(CLOCK_SYN);
		tick++;
	}
	serverInterface->stop();
}

void Client::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time + timeShift);
}

void Client::handleInput() {
	Player &player = world->getPlayer(localClientId);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEWHEEL: {
			if (state == PLAYING) {
				auto block = player.getBlock();
				block += event.wheel.y;
				static const int NUMBER_OF_BLOCKS = 36;
				while (block > NUMBER_OF_BLOCKS) {
					block -= NUMBER_OF_BLOCKS;
				}
				while (block < 1) {
					block += NUMBER_OF_BLOCKS;
				}
				serverInterface->setBlock(block);
			}
			break;
		}
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
				LOG(INFO, "" << buf);
			} else if (event.key.keysym.scancode == SDL_SCANCODE_P) {
				char buf[128];
				sprintf(buf, "timeShift: %ld\n", timeShift = (timeShift + 900000) % 1000000);
				LOG(INFO, "" << buf);
			}
			if (state == PLAYING) {
				switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_ESCAPE:
					menu->update();
					state = IN_MENU;
					world->setPause(true);
					break;
				case SDL_SCANCODE_F:
					if (!world->isPaused()) {
						serverInterface->toggleFly();
					}
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
					menu->update();
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
			} else if (state == IN_MENU) { // if we are in menu
				switch (event.key.keysym.scancode) {
				/*case SDL_SCANCODE_W:
					if (menu->navigateUp()) {
						graphics->setConf(*conf);
						serverInterface->setConf(*conf);
					}
					break;
				case SDL_SCANCODE_S:
					if (menu->navigateDown()) {
						graphics->setConf(*conf);
						serverInterface->setConf(*conf);
					}
				break;
				case SDL_SCANCODE_A:
					if (menu->navigateLeft()) {
						graphics->setConf(*conf);
						serverInterface->setConf(*conf);
					}
				break;
				case SDL_SCANCODE_D:
					if (menu->navigateRight()) {
						graphics->setConf(*conf);
						serverInterface->setConf(*conf);
					}
				break;*/
				case SDL_SCANCODE_ESCAPE:
					menu->apply();
					state = PLAYING;
					world->setPause(false);
					/*for (int z = -5; z < 37; z++) {
						for (int y = -5; y < 37; y++) {
							for (int x = -5; x < 37; x++) {
								world->setBlock(vec3i64(x,y,z), 0, true);
							}
						}
					}
					for (int z = 0; z < 32; z++) {
						for (int y = 0; y < 32; y++) {
							for (int x = 0; x < 32; x++) {
								world->setBlock(vec3i64(x,y,z), 1, true);
							}
						}
					}*/
					break;
				default:
					break;
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if (state == PLAYING && !world->isPaused()) {
				double yaw = player.getYaw();
				double pitch = player.getPitch();
				yaw -= event.motion.xrel / 10.0;
				pitch -= event.motion.yrel / 10.0;
				yaw = fmod(yaw, 360);
				pitch = std::max(pitch, -90.0);
				pitch = std::min(pitch, 90.0);
				serverInterface->setPlayerOrientation(yaw, pitch);
			} else if (state == IN_MENU) {
				int x = event.motion.x;
				int y = graphics->getHeight() - event.motion.y;
				float factor = graphics->getScalingFactor();
				frame->updateMousePosition(x * factor, y * factor);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (state == PLAYING && !world->isPaused()) {
				vec3i64 bc;
				int d;
				bool target = player.getTargetedFace(&bc, &d);
				if (target) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						vec3i64 rbc = bc + DIRS[d].cast<int64>();
						serverInterface->edit(rbc, player.getBlock());
					} else if (event.button.button == SDL_BUTTON_RIGHT) {
						serverInterface->edit(bc, 0);
					}
				}
			} else if (state == IN_MENU){
				if (event.button.button == SDL_BUTTON_LEFT) {
					int x = event.button.x;
					int y = graphics->getHeight() - event.button.y;
					float factor = graphics->getScalingFactor();
					frame->handleMouseClick(x * factor, y * factor);
				}
			}
			break;
		}
	}

	if (menu->check()){
		graphics->setConf(*conf);
		serverInterface->setConf(*conf);
	}

	const uint8 *keyboard = SDL_GetKeyboardState(nullptr);

	int moveInput = 0;
	if (state == PLAYING && !world->isPaused()) {
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
	}
}
