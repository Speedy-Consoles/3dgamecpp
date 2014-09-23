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

static const uint8 MAGIC[4] = {0xaa, 0x0d, 0xbe, 0x15};

enum ClientMessageType:uint8 {
	CONNECTION_REQUEST,
	ECHO_REQUEST,
};

enum ServerMessageType:uint8 {
	CONNECTION_RESPONSE,
	ECHO,
};

struct MessageHeader {
	uint8 magic[4];
	uint8 type;
} __attribute__((__packed__ ));

struct ConnectionRequest {
	// nothing
} __attribute__((__packed__ ));

struct ConnectionResponse {
	bool success;
	uint32 token;
	uint8 id;
} __attribute__((__packed__ ));

struct EchoRequest {
	uint32 token;
} __attribute__((__packed__ ));

struct Echo {
	// nothing
} __attribute__((__packed__ ));

struct Client {
	bool connected;
	uint32 token;
};

class Server {
private:
	World *world;
	int64 time = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTimePoint;
	bool closeRequested = false;

	UDPsocket socket;
	UDPpacket *outPacket;
	UDPpacket *inPacket;

	Client clients[MAX_CLIENTS];
public:
	Server(uint16 port);
	~Server();
	void run();
	void sync(int perSecond);
private:
	void receive();
	bool getNewId(uint8 *id);
	void writeBytes(uint8 **cursor, const uint8 *data, size_t len);
	template <typename T> void writeBytes(uint8 **cursor, const T *data);
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
	socket = SDLNet_UDP_Open(port);
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
	}
	inPacket = SDLNet_AllocPacket(1024 * 64);
	outPacket = SDLNet_AllocPacket(1024 * 64);
}

Server::~Server() {
	SDLNet_UDP_Close(socket);
	delete world;
	SDLNet_FreePacket(inPacket);
	SDLNet_FreePacket(outPacket);
}

void Server::run() {
	LOG(INFO, "Running server");

	if (!socket) {
		LOG(FATAL, "Socket not open!");
		return;
	}

	startTimePoint = high_resolution_clock::now();
	time = 0;
	int tick = 0;
	while (!closeRequested) {
		receive();

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

void Server::receive() {
	uint8 *outDataCursor;
	MessageHeader outHeader;
	memcpy(outHeader.magic, MAGIC, sizeof MAGIC);
	int rc;
	while ((rc = SDLNet_UDP_Recv(socket, inPacket)) > 0) {
		uint8 *inDataCursor = inPacket->data;
		const uint8 *dataEnd = inPacket->data + inPacket->len;
		if ((size_t) (dataEnd - inDataCursor) < sizeof (MessageHeader)) continue;
		MessageHeader inHeader;
		// reading message header
		memcpy(&inHeader, inDataCursor, sizeof (MessageHeader));
		inDataCursor += sizeof (MessageHeader);
		// checking magic
		if (memcmp(inHeader.magic, MAGIC, sizeof (inHeader.magic)) != 0) continue;

		if (inHeader.type == CONNECTION_REQUEST) {
			ConnectionResponse msg;
			uint8 id;
			if (getNewId(&id)) {
				uint32 token = 0; //TODO
				clients[id].connected = true;
				clients[id].token = token;
				SDLNet_UDP_Bind(socket, id, &inPacket->address);
				msg.success = true;
				msg.token = token;
				msg.id = id;
			} else {
				msg.success = false;
				msg.token = 0;
				msg.id = 0;
			}
			outHeader.type = CONNECTION_RESPONSE;
			outDataCursor = outPacket->data;
			writeBytes(&outDataCursor, &outHeader);
			writeBytes(&outDataCursor, &msg);
			outPacket->len = outDataCursor - outPacket->data;
			outPacket->address = inPacket->address;
			SDLNet_UDP_Send(socket, -1, outPacket);
		} else {
			// TODO
			switch (inHeader.type) {
			case ECHO_REQUEST:
				// TODO
				break;
			default:
				break;
			}
		}
	}

	if (rc < 0)
		LOG(ERROR, "Error while receiving inPacket!");
}

bool Server::getNewId(uint8 *id) {
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		if (!clients[i].connected) {
			*id = i;
			return true;
		}
	}
	return false;
}

void Server::writeBytes(uint8 **cursor, const uint8 *data, size_t len) {
	memcpy(*cursor, data, len);
	*cursor += len;
}

template <typename T>
void Server::writeBytes(uint8 **cursor, const T *data) {
	writeBytes(cursor, reinterpret_cast<const uint8 *>(data), sizeof (T));
}
