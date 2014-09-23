#include "game/world.hpp"
#include "io/logging.hpp"
#include "std_types.hpp"
#include "util.hpp"
#include "constants.hpp"

#include <SDL2/SDL_net.h>
#include <chrono>
#include <thread>

using namespace std;

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("server")

static const uint8 MAGIC[4] = {0xaa, 0x0d, 0xbe, 0x15};

enum ClientMessageType:uint8 {
	CONNECTION_REQUEST,
	ECHO_REQUEST,
};

enum ServerMessageType:uint8 {
	CONNECTION_RESPONSE,
	ECHO_RESPONSE,
	CONNECTION_RESET,
};

struct MessageHeader {
	uint8 magic[4];
	uint8 type;
} __attribute__((__packed__ ));

struct ClientMessageHeader {
	uint32 token;
} __attribute__((__packed__ ));

struct ConnectionResponse {
	bool success;
	uint8 id;
	uint32 token;
} __attribute__((__packed__ ));


struct Client {
	bool connected;
	uint32 token;
	uint64 timeOfLastPacket;
};

class Server {
private:
	bool closeRequested = false;
	World *world = nullptr;

	// time keeping
	// precise time uses the best hardware clock available (microseconds)
	int64 time = 0;
	chrono::time_point<chrono::high_resolution_clock> startTimePoint;

	// approximate time uses the standard clock (milliseconds)
	uint64 approxTime = 0;
	chrono::time_point<chrono::steady_clock> approxStartTimePoint;

	// connections
	UDPsocket socket;
	UDPpacket *outPacket;
	UDPpacket *inPacket;

	Client clients[MAX_CLIENTS];

	uint64 timeout = 5000; // 30 seconds

public:
	Server(uint16 port);
	~Server();

	void run();
	void sync(int perSecond);

	int64 getPreciseTime();
	uint64 getApproxTime();

private:

	void receive();
	void checkInactive();

	void handleConnectionRequest(const IPaddress *);
	void handleClientMessage(uint8 type, uint8 id, const uint8 *data, const uint8 *dataEnd);

	void disconnect(uint8 id);

	bool getNewId(uint8 *id);

	template <typename T>
	void write(uint8 **cursor, const T *data);
	void writeHeader(uint8 **cursor, uint8 type);
	void writeBytes(uint8 **cursor, const uint8 *data, size_t len);

	template <typename T>
	bool read(const uint8 **cursor, T *data, const uint8 *end);
	bool readBytes(const uint8 **cursor, uint8 *data, size_t len, const uint8 *end);
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
	LOG(INFO, "Starting server on port " << port);
	world = new World();
	socket = SDLNet_UDP_Open(port);
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
		clients[i].timeOfLastPacket = 0;
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
	if (!socket) {
		LOG(FATAL, "Socket not open!");
		return;
	}

	approxStartTimePoint = chrono::steady_clock::now();
	startTimePoint = chrono::high_resolution_clock::now();
	time = 0;
	approxTime = 0;
	int tick = 0;
	while (!closeRequested) {
		receive();
		checkInactive();

		world->tick(tick, -1);
		sync(TICK_SPEED);
		tick++;
	}
}

void Server::sync(int perSecond) {
	time = time + 1000000 / perSecond;
	microseconds duration(std::max(0,
			(int) (time - getPreciseTime())));
	std::this_thread::sleep_for(duration);
}

int64 Server::getPreciseTime() {
	auto diff = chrono::high_resolution_clock::now() - startTimePoint;
	auto micros = chrono::duration_cast<std::chrono::microseconds>(diff);
	return micros.count();
}

uint64 Server::getApproxTime() {
	auto diff = chrono::steady_clock::now() - approxStartTimePoint;
	auto millis = chrono::duration_cast<std::chrono::milliseconds>(diff);
	return millis.count();
}

void Server::receive() {
	int rc;
	while ((rc = SDLNet_UDP_Recv(socket, inPacket)) > 0) {
		LOG(TRACE, "Received message of length " << inPacket->len);

		// read the message header
		const uint8 *inDataCursor = inPacket->data;
		const uint8 *dataEnd = inPacket->data + inPacket->len;

		MessageHeader inHeader;
		if (!read(&inDataCursor, &inHeader, dataEnd)) {
			LOG(DEBUG, "Message too short for basic protocol header");
			continue;
		}

		// checking magic
		if (memcmp(inHeader.magic, MAGIC, sizeof (inHeader.magic)) != 0) {
			LOG(DEBUG, "Incorrect magic");
			continue;
		}

		if (inHeader.type == CONNECTION_REQUEST) {
			handleConnectionRequest(&inPacket->address);
		} else {
			int channel = inPacket->channel;
			if (0 <= channel && channel < (int) MAX_CLIENTS) {
				uint8 id = channel;

				ClientMessageHeader message;
				if (!read(&inDataCursor, &message, dataEnd)) {
					LOG(DEBUG, "Message stopped abruptly");
					continue;
				}

				if (clients[id].token != message.token) {
					LOG(DEBUG, "Invalid token for Player " << (int) id);
					continue;
				}

				clients[id].timeOfLastPacket = getApproxTime();

				handleClientMessage(inHeader.type, id, inDataCursor, dataEnd);
			} else if (channel == -1) {
				LOG(DEBUG, "Unknown address");
			} else {
				LOG(WARNING, "Incorrect channel");
			}
		}
	}

	if (rc < 0)
		LOG(ERROR, "Error while receiving inPacket!");
}

void Server::checkInactive() {
	uint64 approxTimeNow = getApproxTime();
	for (uint8 id = 0; id < MAX_CLIENTS; ++id) {
		if (!clients[id].connected)
			continue;

		if (approxTimeNow - clients[id].timeOfLastPacket > timeout) {
			LOG(INFO, "Player " << (int) id << " timed out");
			disconnect(id);
		}
	}
}

void Server::handleConnectionRequest(const IPaddress *address) {
	uint8 *host_bytes = (uint8 *) &address->host;
	uint8 *port_bytes = (uint8 *) &address->port;
	LOG(INFO, "New connection from IP "
			<< (int) host_bytes[0] << "." << (int) host_bytes[1] << "."
			<< (int) host_bytes[2] << "." << (int) host_bytes[3]
			<< " and port " << (((int) port_bytes[0] << 8) | port_bytes[1])
	);

	// handle message
	ConnectionResponse msg;
	uint8 id;
	if (getNewId(&id)) {
		uint32 token = 0; //TODO
		clients[id].connected = true;
		clients[id].token = token;
		clients[id].timeOfLastPacket = getApproxTime();
		SDLNet_UDP_Bind(socket, id, address);
		msg.success = true;
		msg.token = token;
		msg.id = id;
		LOG(INFO, "Player " << (int) id << " connected");
		LOG(INFO, "Token is " << token);
	} else {
		msg.success = false;
		msg.token = 0;
		msg.id = 0;
		LOG(INFO, "Server is full");
	}

	// prepare answer
	uint8 *outDataCursor = outPacket->data;
	writeHeader(&outDataCursor, CONNECTION_RESPONSE);
	write(&outDataCursor, &msg);
	outPacket->len = outDataCursor - outPacket->data;
	// send to specific address, in case the connection got rejected
	outPacket->address = inPacket->address;
	SDLNet_UDP_Send(socket, -1, outPacket);
}

void Server::handleClientMessage(uint8 type, uint8 id, const uint8 *data, const uint8 *dataEnd) {
	LOG(TRACE, "Message " << (int) type << " for Player " << (int) id << " accepted");
	switch (type) {
	case ECHO_REQUEST: {
		uint8 *outDataCursor = outPacket->data;
		writeHeader(&outDataCursor, ECHO_RESPONSE);
		writeBytes(&outDataCursor, data, dataEnd - data);
		outPacket->len = outDataCursor - outPacket->data;
		SDLNet_UDP_Send(socket, id, outPacket);
		break;
	}
	default:
		break;
	}
}

void Server::disconnect(uint8 id) {
	clients[id].connected = false;

	uint8 *cursor = outPacket->data;
	writeHeader(&cursor, CONNECTION_RESET);
	outPacket->len = cursor - outPacket->data;
	SDLNet_UDP_Send(socket, id, outPacket);
	SDLNet_UDP_Unbind(socket, id);

	LOG(INFO, "Player " << (int) id << " disconnected");
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

template <typename T>
void Server::write(uint8 **cursor, const T *data) {
	auto data_ptr = reinterpret_cast<const uint8 *>(data);
	writeBytes(cursor, data_ptr, sizeof (T));
}

void Server::writeHeader(uint8 **cursor, uint8 type) {
	MessageHeader header;
	memcpy(header.magic, MAGIC, sizeof MAGIC);
	header.type = type;
	write(cursor, &header);
}

void Server::writeBytes(uint8 **cursor, const uint8 *data, size_t len) {
	memcpy(*cursor, data, len);
	*cursor += len;
}

template <typename T>
bool Server::read(const uint8 **cursor, T *data, const uint8 *end) {
	auto data_ptr = reinterpret_cast<uint8 *>(data);
	return readBytes(cursor, data_ptr, sizeof (T), end);
}

bool Server::readBytes(const uint8 **cursor, uint8 *data, size_t len, const uint8 *end) {
	if (end && (size_t) (end - *cursor) < len) {
		return false;
	}
	memcpy(data, *cursor, len);
	*cursor += len;
	return true;
}
