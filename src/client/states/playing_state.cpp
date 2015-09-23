#include "playing_state.hpp"

#include "../../shared/game/character.hpp"
#include "client/client.hpp"
#include "client/events.hpp"
#include "client/config.hpp"
#include "client/server_interface.hpp"
#include "client/client_chunk_manager.hpp"
#include "client/gfx/graphics.hpp"
#include "client/gfx/gl2/gl2_renderer.hpp"
#include "client/gfx/gl3/gl3_renderer.hpp"

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"
#include "shared/block_manager.hpp"
#include "shared/block_utils.hpp"
#include "shared/saves.hpp"

#include "menu_state.hpp"

PlayingState::PlayingState(State *parent, Client *client) :
	State(parent, client)
{
	if (client->conf->render_backend == RenderBackend::OGL_3) {
		client->renderer = std::unique_ptr<GL3Renderer>(new GL3Renderer(client));
	} else {
		client->renderer = std::unique_ptr<GL2Renderer>(new GL2Renderer(client));
	}
}

PlayingState::~PlayingState() {
	client->serverInterface.reset();
	client->renderer.reset();
	client->world.reset();
	client->chunkManager.reset();
	client->blockManager.reset();
	client->save.reset();
}

void PlayingState::hide() {
	hidden = true;
}

void PlayingState::unhide() {
	hidden = false;
	client->getGraphics()->grabMouse(true);
	client->setStateId(Client::PLAYING);
}

void PlayingState::update() {
	State::update();

	ServerInterface *serverInterface = client->getServerInterface();

	if (!hidden) {
		const uint8 *keyboard = SDL_GetKeyboardState(nullptr);
		int moveInput = 0;

		if (keyboard[SDL_SCANCODE_D])
			moveInput |= Character::MOVE_INPUT_FLAG_STRAFE_RIGHT;
		if (keyboard[SDL_SCANCODE_A])
			moveInput |= Character::MOVE_INPUT_FLAG_STRAFE_LEFT;

		if (keyboard[SDL_SCANCODE_SPACE])
			moveInput |= Character::MOVE_INPUT_FLAG_FLY_UP;
		if (keyboard[SDL_SCANCODE_LCTRL])
			moveInput |= Character::MOVE_INPUT_FLAG_FLY_DOWN;

		if (keyboard[SDL_SCANCODE_W])
			moveInput |= Character::MOVE_INPUT_FLAG_MOVE_FORWARD;
		if (keyboard[SDL_SCANCODE_S])
			moveInput |= Character::MOVE_INPUT_FLAG_MOVE_BACKWARD;

		if (keyboard[SDL_SCANCODE_LSHIFT])
			moveInput |= Character::MOVE_INPUT_FLAG_SPRINT;

		serverInterface->setPlayerMoveInput(moveInput);
	} else {
		serverInterface->setPlayerMoveInput(0);
	}

	client->getChunkManager()->tick();
	serverInterface->tick();

#ifndef NO_GRAPHICS
	if (getCurrentTime() < client->time + client->timeShift + seconds(1) / TICK_SPEED) {
		client->renderer->tick();
		client->renderer->render();
	}
#endif
}

void PlayingState::handle(const Event &e) {
	ServerInterface *serverInterface = client->getServerInterface();
	const Character &character = client->getLocalCharacter();

	switch (e.type) {

	case EventType::MOUSE_MOTION: {
		int yaw = character.getYaw();
		int  pitch = character.getPitch();
		yaw -= (int)round(e.event.motion.xrel * 10.0f);
		pitch -= (int)round(e.event.motion.yrel * 10.0f);
		yaw = cycle(yaw, 36000);
		pitch = std::max(pitch, -9000);
		pitch = std::min(pitch, 9000);
		serverInterface->setCharacterOrientation(yaw, pitch);
		break;
	} // case MOUSE_MOTION

	case EventType::MOUSE_BUTTON_PRESSED: {
		vec3i64 bc;
		int d;
		bool target = character.getTargetedFace(&bc, &d);
		if (target) {
			if (e.event.button.button == SDL_BUTTON_LEFT) {
				vec3i64 rbc = bc + DIRS[d].cast<int64>();
				serverInterface->placeBlock(rbc, character.getBlock());
			} else if (e.event.button.button == SDL_BUTTON_RIGHT) {
				serverInterface->placeBlock(bc, 0);
			}
		}
		break;
	} // case MOUSE_BUTTON_PRESSED

	case EventType::MOUSE_WHEEL: {
		auto block = character.getBlock();
		block += e.event.wheel.y;
		static const int NUMBER_OF_BLOCKS = client->getBlockManager()->getNumberOfBlocks();
		while (block > NUMBER_OF_BLOCKS) {
			block -= NUMBER_OF_BLOCKS;
		}
		while (block < 1) {
			block += NUMBER_OF_BLOCKS;
		}
		serverInterface->setSelectedBlock(block);
		break;
	} // case MOUSE_WHEEL

	case EventType::KEYBOARD_PRESSED:
		switch (e.event.key.keysym.scancode) {
		case SDL_SCANCODE_ESCAPE:
			client->pushState(new MenuState(this, client));
			client->getGraphics()->grabMouse(false);
			break;
		case SDL_SCANCODE_F:
			serverInterface->toggleFly();
			break;
		case SDL_SCANCODE_M: {
			GraphicsConf c = client->getConf();
			switch (c.aa) {
			case AntiAliasing::NONE:    c.aa = AntiAliasing::MSAA_2;  break;
			case AntiAliasing::MSAA_2:  c.aa = AntiAliasing::MSAA_4;  break;
			case AntiAliasing::MSAA_4:  c.aa = AntiAliasing::MSAA_8;  break;
			case AntiAliasing::MSAA_8:  c.aa = AntiAliasing::MSAA_16; break;
			case AntiAliasing::MSAA_16: c.aa = AntiAliasing::NONE;    break;
			}
			client->setConf(c);
			client->getMenu()->update();
			break;
		}
		case SDL_SCANCODE_F11: {
			GraphicsConf c = client->getConf();
			c.fullscreen = !c.fullscreen;
			client->setConf(c);
			break;
		}
		case SDL_SCANCODE_F3:
			client->setDebugOn(!client->isDebugOn());
			break;
		default:
			parent->handle(e);
			break;
		} // switch scancode
		break;

	default:
		parent->handle(e);
		break;

	} // switch event type
}
