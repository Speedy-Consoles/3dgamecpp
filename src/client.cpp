#include <iostream>
#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "util.hpp"
#include "constants.hpp"
#include "client.hpp"
#include "local_server_interface.hpp"

using namespace std::chrono;

int main() {
	initUtil();
	Client client;
	client.run();

	return 0;
}

Client::Client() {
	world = new World();

	//serverInterface = RemoteServerInterface(args[0], world);
	serverInterface = new LocalServerInterface(world, 42);

	localClientID = serverInterface->getLocalClientID();

	graphics = new Graphics(world, localClientID);
}

Client::~Client() {
	delete graphics;
	// world must be deleted before server interface
	delete world;
	delete serverInterface;
}

void Client::run() {
	startTimePoint = high_resolution_clock::now();
	time = 0;
	int tick = 0;
	while (!closeRequested) {
		handleInput();

		serverInterface->sendInput();
		serverInterface->receive(time + 200000);

		world->tick(tick, localClientID);

		if (time + 1000000 / TICK_SPEED > getMicroTimeSince(startTimePoint))
			graphics->tick();

		sync(TICK_SPEED);
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
				graphics->grab();
				break;
			case SDL_SCANCODE_F:
				serverInterface->togglePlayerFly();
				break;
			default:
				//printf("unknown key: ", event.key.keysym);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			if (graphics->isGrabbed() && player.isValid()) {
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

	if (keyboard[SDL_SCANCODE_D] && !keyboard[SDL_SCANCODE_A])
		moveInput |= Player::MOVE_INPUT_FLAG_STRAFE_RIGHT;
	else if (keyboard[SDL_SCANCODE_A] && !keyboard[SDL_SCANCODE_D])
		moveInput |= Player::MOVE_INPUT_FLAG_STRAFE_LEFT;

	if (keyboard[SDL_SCANCODE_SPACE] && !keyboard[SDL_SCANCODE_LCTRL])
		moveInput |= Player::MOVE_INPUT_FLAG_FLY_UP;
	else if (keyboard[SDL_SCANCODE_LCTRL] && !keyboard[SDL_SCANCODE_SPACE])
		moveInput |= Player::MOVE_INPUT_FLAG_FLY_DOWN;

	if (keyboard[SDL_SCANCODE_W] && !keyboard[SDL_SCANCODE_S])
		moveInput |= Player::MOVE_INPUT_FLAG_MOVE_FORWARD;
	else if (keyboard[SDL_SCANCODE_S] && !keyboard[SDL_SCANCODE_W])
		moveInput |= Player::MOVE_INPUT_FLAG_MOVE_BACKWARD;

	serverInterface->setPlayerMoveInput(moveInput);

	if (player.isValid()) {
		player.setMoveInput(moveInput);
	}

	serverInterface->sendInput();
}
