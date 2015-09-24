#include "game_server.hpp"

#include "shared/engine/std_types.hpp"
#include "shared/block_utils.hpp"
#include "shared/net.hpp"

static logging::Logger logger("gserver");

GameServer::GameServer(Server *server) : server(server) {
	LOG_INFO(logger) << "Creating game server";
	world = server->getWorld();
}

GameServer::~GameServer() {
	// nothing
}

void GameServer::tick() {
	world->tick();

	//TODO use makeSnapshot
	sendSnapshots(server->getTick());
}

void GameServer::onPlayerJoin(int id, PlayerInfo &info) {
	players[id].valid = true;
	players[id].name = info.name;
	world->addCharacter(id);

	PlayerJoinEvent pje;
	pje.id = id;
	server->broadcast(pje, true);

	LOG_INFO(logger) << players[id].name << " joined the game";
}

void GameServer::onPlayerLeave(int id, DisconnectReason reason) {
	players[id].valid = false;
	world->deleteCharacter(id);

	PlayerLeaveEvent ple;
	ple.id = id;
	server->broadcast(ple, true);

	switch (reason) {
	case TIMEOUT:
		LOG_INFO(logger) <<  players[id].name << " timed out";
		break;
	case CLIENT_LEAVE:
		LOG_INFO(logger) <<  players[id].name << " disconnected";
		break;
	default:
		LOG_WARNING(logger) << "Unexpected DisconnectReason " << reason;
		break;
	}
}

void GameServer::onPlayerInput(int id, PlayerInput &input) {
	int yaw = input.yaw;
	int pitch = input.pitch;
	world->getCharacter(id).setOrientation(yaw, pitch);
	world->getCharacter(id).setMoveInput(input.moveInput);
	world->getCharacter(id).setFly(input.flying);
}

void GameServer::onChunkRequest(ChunkRequest &request, ChunkMessageJob job) {
	// TODO
}

void GameServer::sendSnapshots(int tick) {
	Snapshot snapshot;
	snapshot.tick = tick;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		CharacterSnapshot &characterSnapshot = *(snapshot.characterSnapshots + i);
		Character &character = world->getCharacter(i);
		if (!character.isValid()) {
			characterSnapshot.valid = false;
			characterSnapshot.pos = vec3i64(0, 0, 0);
			characterSnapshot.vel = vec3d(0.0, 0.0, 0.0);
			characterSnapshot.yaw = 0;
			characterSnapshot.pitch = 0;
			characterSnapshot.moveInput = 0;
			characterSnapshot.isFlying = false;
			continue;
		}
		characterSnapshot.valid = true;
		characterSnapshot.pos = character.getPos();
		characterSnapshot.vel = character.getVel();
		characterSnapshot.yaw = character.getYaw();
		characterSnapshot.pitch = character.getPitch();
		characterSnapshot.moveInput = character.getMoveInput();
		characterSnapshot.isFlying = character.getFly();
	}

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (!players[i].valid)
			continue;
		snapshot.localId = i;
		server->send(snapshot, i, false);
	}
}
