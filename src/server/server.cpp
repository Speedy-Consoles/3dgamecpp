#include "socket.hpp"
#include "game/world.hpp"
#include "io/logging.hpp"
#include "std_types.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "time.hpp"

#include <boost/asio.hpp>
#include <thread>
#include <future>

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

using namespace my::time;
using namespace my::net;

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("server")

static const uint8 MAGIC[4] = {0xaa, 0x0d, 0xbe, 0x15};

enum ClientMessageType : uint8 {
	CONNECTION_REQUEST,
	ECHO_REQUEST,
};

enum ServerMessageType : uint8 {
	CONNECTION_ACCEPTED,
	CONNECTION_REJECTED,
	CONNECTION_TIMEOUT,
	CONNECTION_RESET,
	ECHO_RESPONSE,
};

enum ConnectionRejectionReason : uint8 {
	SERVER_FULL,
	DUPLICATE_ENDPOINT,
};

struct MessageHeader {
	uint8 magic[4];
	uint8 type;
} __attribute__((__packed__ ));

struct ClientMessageHeader {
	uint32 token;
} __attribute__((__packed__ ));

struct ConnectionAcceptedResponse {
	uint8 id;
	uint32 token;
} __attribute__((__packed__ ));

struct ConnectionRejectedResponse {
	uint8 reason;
} __attribute__((__packed__ ));

struct Client {
	bool connected;
	udp::endpoint endpoint;
	uint32 token;
	time_t timeOfLastPacket;
};

class Server {
private:
	bool closeRequested = false;
	World *world = nullptr;

	Client clients[MAX_CLIENTS];

	uint16 port = 0;
	time_t timeout = my::time::seconds(10); // 10 seconds

	Buffer inBuf;
	Buffer outBuf;

	// time keeping
	// precise time uses the best hardware clock available (microseconds)
	time_t time = 0;

	// our listening socket
	my::net::ios_t ios;
	my::net::Socket socket;

public:
	Server(uint16 port);
	~Server();

	void run();

private:

	template <typename T>
	bool receiveFor(T dur);
	void signalReadyToReceive();
	void handle(const Packet &p);
	void checkInactive();

	void handleConnectionRequest(const endpoint_t &);
	void handleClientMessage(uint8 type, uint8 id, const char *data, const char *dataEnd);

	void disconnect(uint8 id);

	template <typename T>
	void write(char **cursor, const T *data);
	void writeHeader(char **cursor, uint8 type);
	void writeBytes(char **cursor, const char *data, size_t len);

	template <typename T>
	bool read(const char **cursor, T *data, const char *end);
	bool readBytes(const char **cursor, char *data, size_t len, const char *end);
};

int main(int argc, char *argv[]) {
	initLogging("logging_srv.conf");

	LOG(INFO, "Starting server");
	LOG(TRACE, "Trace enabled");

	initUtil();
	Server server(8547);

	try {
		server.run();
	} catch (std::exception &e) {
		LOG(FATAL, "Exception: " << e.what());
		return -1;
	}

	return 0;

}

Server::Server(uint16 port) :
	port(port),
	inBuf(1024*64),
	outBuf(1024*64),
	ios(),
	socket(ios)
{
	LOG(INFO, "Creating Server");
	world = new World();
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
		clients[i].timeOfLastPacket = 0;
	}
}

Server::~Server() {
	delete world;
}

void Server::run() {
	LOG(INFO, "Starting Server on port " << port);

	time = my::time::get();

	if (socket.getSystemError()) {
		LOG(FATAL, "Could not create listening socket");
		return;
	}

	socket.open();
	if (socket.getSystemError()) {
		LOG(FATAL, "Could not create listening socket");
		return;
	}

	socket.bind(udp::endpoint(udp::v4(), port));

	if (socket.getSystemError()) {
		LOG(FATAL, "Could not bind listening socket");
		return;
	}

	// this will make sure our async_recv calls get handled
	asio::io_service::work w(ios);
	future<void> f = async(launch::async, [this]{ ios.run(); });

	int tick = 0;
	while (!closeRequested) {
		time_t remTime;
		while ((remTime = time - get() + seconds(1) / TICK_SPEED) > 0) {
			inBuf.clear();
			Packet p = {&inBuf};
			socket.receiveFor(&p, remTime);
			if (socket.getErrorCode() == Socket::OK)
				handle(p);
		}

		world->tick(tick, -1);
		tick++;
		time += seconds(1) / TICK_SPEED;

		checkInactive();
	}
}

void Server::handle(const Packet &p) {
	LOG(TRACE, "Received message of length " << p.buf->rSize());

	// read the message header
	const char *inDataCursor = p.buf->rBegin();
	const char *dataEnd = p.buf->rEnd();

	MessageHeader header;
	if (!read(&inDataCursor, &header, dataEnd)) {
		LOG(DEBUG, "Message too short for basic protocol header");
		return;
	}

	// checking magic
	if (memcmp(header.magic, MAGIC, sizeof (header.magic)) != 0) {
		LOG(DEBUG, "Incorrect magic");
		return;
	}

	if (header.type == CONNECTION_REQUEST) {
		handleConnectionRequest(p.endpoint);
	} else {
		int player = -1;
		for (int id = 0; id < (int) MAX_CLIENTS; ++id) {
			if (clients[id].endpoint == p.endpoint) {
				player =  id;
				break;
			}
		}

		if (player >= 0 && clients[player].connected) {
			uint8 id = player;

			ClientMessageHeader message;
			if (!read(&inDataCursor, &message, dataEnd)) {
				LOG(DEBUG, "Message stopped abruptly");
				return;
			}

			if (clients[id].token != message.token) {
				LOG(DEBUG, "Invalid token for Player " << (int) id);
				return;
			}

			clients[id].timeOfLastPacket = my::time::get();

			handleClientMessage(header.type, id, inDataCursor, dataEnd);
		} else {
			LOG(DEBUG, "Unknown remote address");

			outBuf.clear();
			char *cursor = outBuf.wBegin();
			writeHeader(&cursor, CONNECTION_RESET);
			Packet outPacket{&outBuf, p.endpoint};
			socket.send(outPacket);
		}
	}
}

void Server::handleConnectionRequest(const endpoint_t &endpoint) {
	LOG(INFO, "New connection from IP "
			<< endpoint.address().to_string()
			<< " and port " << endpoint.port()
	);

	// find a free spot for the new player
	int newPlayer = -1;
	int duplicate = -1;
	for (int i = 0; i < (int) MAX_CLIENTS; i++) {
		bool connected = clients[i].connected;
		if (newPlayer == -1 && !connected) {
			newPlayer = i;
		}
		if (connected && clients[i].endpoint == endpoint) {
			duplicate = i;
		}
	}

	// check for problems
	bool failure = false;
	uint8 reason;
	if (newPlayer == -1) {
		LOG(INFO, "Server is full");
		failure = true;
		reason = SERVER_FULL;
	} else if (duplicate != -1) {
		LOG(INFO, "Duplicate remote endpoint with player " << duplicate);
		failure = true;
		reason = DUPLICATE_ENDPOINT;
	}

	// build response
	outBuf.clear();
	char *outDataCursor = outBuf.wBegin();
	if (!failure) {
		uint8 id = (uint) newPlayer;
		uint32 token = 0; //TODO

		clients[id].connected = true;
		clients[id].endpoint = endpoint;
		clients[id].token = token;
		clients[id].timeOfLastPacket = my::time::get();

		ConnectionAcceptedResponse msg;
		msg.token = token;
		msg.id = id;
		writeHeader(&outDataCursor, CONNECTION_ACCEPTED);
		write(&outDataCursor, &msg);
		LOG(INFO, "Player " << (int) id << " connected with token "
				<< token);
	} else {
		ConnectionRejectedResponse msg;
		msg.reason = reason;
		writeHeader(&outDataCursor, CONNECTION_REJECTED);
		write(&outDataCursor, &msg);
	}

	// send response
	Packet outPacket{&outBuf, endpoint};
	socket.send(outPacket);
}

void Server::handleClientMessage(uint8 type, uint8 id, const char *data, const char *dataEnd) {
	LOG(TRACE, "Message " << (int) type << " for Player " << (int) id << " accepted");
	switch (type) {
	case ECHO_REQUEST: {
		outBuf.clear();
		char *outDataCursor = outBuf.wBegin();
		writeHeader(&outDataCursor, ECHO_RESPONSE);
		writeBytes(&outDataCursor, data, dataEnd - data);
		Packet outPacket{&outBuf, clients[id].endpoint};
		socket.send(outPacket);
		break;
	}
	default:
		break;
	}
}

void Server::checkInactive() {
	for (uint8 id = 0; id < MAX_CLIENTS; ++id) {
		if (!clients[id].connected)
			continue;

		if (my::time::get() - clients[id].timeOfLastPacket > timeout) {
			LOG(INFO, "Player " << (int) id << " timed out");

			outBuf.clear();
			char *cursor = outBuf.wBegin();
			writeHeader(&cursor, CONNECTION_TIMEOUT);
			Packet outPacket{&outBuf, clients[id].endpoint};
			socket.send(outPacket);

			disconnect(id);
		}
	}
}

void Server::disconnect(uint8 id) {
	LOG(INFO, "Player " << (int) id << " disconnected");
	clients[id].connected = false;
}

template <typename T>
void Server::write(char **cursor, const T *data) {
	auto data_ptr = reinterpret_cast<const char *>(data);
	writeBytes(cursor, data_ptr, sizeof (T));
}

void Server::writeHeader(char **cursor, uint8 type) {
	MessageHeader header;
	memcpy(header.magic, MAGIC, sizeof MAGIC);
	header.type = type;
	write(cursor, &header);
}

void Server::writeBytes(char **cursor, const char *data, size_t len) {
	memcpy(*cursor, data, len);
	*cursor += len;
}

template <typename T>
bool Server::read(const char **cursor, T *data, const char *end) {
	auto data_ptr = reinterpret_cast<char *>(data);
	return readBytes(cursor, data_ptr, sizeof (T), end);
}

bool Server::readBytes(const char **cursor, char *data, size_t len, const char *end) {
	if (end && (size_t) (end - *cursor) < len) {
		return false;
	}
	memcpy(data, *cursor, len);
	*cursor += len;
	return true;
}