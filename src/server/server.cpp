#include "game/world.hpp"
#include "io/logging.hpp"
#include "std_types.hpp"
#include "util.hpp"
#include "constants.hpp"

#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

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
	uint64 timeOfLastPacket;
};

class Server {
private:
	bool closeRequested = false;
	World *world = nullptr;

	Client clients[MAX_CLIENTS];

	uint16 port = 0;
	uint64 timeout = 10000; // 10 seconds

	size_t inBufferSize = 0;
	size_t outBufferSize = 0;
	char *inBuffer = nullptr;
	char *outBuffer = nullptr;

	// time keeping
	// precise time uses the best hardware clock available (microseconds)
	int64 time = 0;
	chrono::time_point<chrono::high_resolution_clock> startTimePoint;

	// approximate time uses the standard clock (milliseconds)
	uint64 approxTime = 0;
	chrono::time_point<chrono::steady_clock> approxStartTimePoint;

	// our listening socket
	asio::io_service ios;
	udp::socket socket;

	// used for handling asynchronous receives
	udp::endpoint remoteEndpoint;
	std::future<size_t> newPacketFuture;
	std::promise<size_t> newPacketPromise;

public:
	Server(uint16 port);
	~Server();

	void run();

private:

	void signalReadyToReceive();
	void receive();
	void checkInactive();

	void handleConnectionRequest();
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

Server::Server(uint16 port) : port(port), socket(ios) {
	LOG(INFO, "Creating Server");
	world = new World();
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
		clients[i].timeOfLastPacket = 0;
	}
	inBufferSize = 1024 * 64;
	outBufferSize = 1024 * 64;
	inBuffer = new char[inBufferSize];
	outBuffer = new char[outBufferSize];
}

Server::~Server() {
	if (socket.is_open())
		socket.close();
	delete[] inBuffer;
	delete[] outBuffer;
	delete world;
}

void Server::run() {
	LOG(INFO, "Starting Server on port " << port);

	approxStartTimePoint = chrono::steady_clock::now();
	startTimePoint = chrono::high_resolution_clock::now();
	time = 0;
	approxTime = 0;

	socket.open(udp::v4());
	socket.bind(udp::endpoint(udp::v4(), port));

	// this will make sure our async_recv calls get handled
	asio::io_service::work w(ios);
	future<void> f = async(launch::async, [this]{ ios.run(); });

	signalReadyToReceive();

	int tick = 0;
	while (!closeRequested) {
		switch (newPacketFuture.wait_for(chrono::milliseconds(5))) {
		case future_status::ready:
			receive();
			signalReadyToReceive();
			break;
		case future_status::deferred:
			LOG(ERROR, "Deferred future");
			break;
		default:
			break;
		}

		checkInactive();

		if (time - getMicroTimeSince(startTimePoint) > 1000000 / TICK_SPEED) {
			world->tick(tick, -1);
			time += 1000000 / TICK_SPEED;
			tick++;
		}
	}
}

void Server::signalReadyToReceive() {
	auto buf = asio::buffer(inBuffer, inBufferSize);

	auto lambda = [this](const system::error_code &error, size_t size) {
		if (!error) {
			LOG(TRACE, "Asynchronous message received");
			newPacketPromise.set_value(size);
		} else if (error == boost::asio::error::message_size) {
			LOG(WARNING, "Message of length " << size
					<< " did not fit into input buffer");
			newPacketPromise.set_value(0);
		} else {
			LOG(ERROR, "Could not receive message");
			newPacketPromise.set_value(0);
		}
	};

	newPacketPromise = std::promise<size_t>();
	newPacketFuture = newPacketPromise.get_future();
	socket.async_receive_from(buf, remoteEndpoint, lambda);
}

void Server::receive() {
	size_t newPacketSize = newPacketFuture.get();
	LOG(TRACE, "Received message of length " << newPacketSize);

	// read the message header
	const char *inDataCursor = inBuffer;
	const char *dataEnd = inDataCursor + newPacketSize;

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
		handleConnectionRequest();
	} else {
		int player = -1;
		for (int id = 0; id < (int) MAX_CLIENTS; ++id) {
			if (clients[id].endpoint == remoteEndpoint) {
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

			clients[id].timeOfLastPacket = getMilliTimeSince(approxStartTimePoint);

			handleClientMessage(header.type, id, inDataCursor, dataEnd);
		} else {
			LOG(DEBUG, "Unknown remote address");

			char *cursor = outBuffer;
			writeHeader(&cursor, CONNECTION_RESET);
			auto buffer = asio::buffer(outBuffer, cursor - outBuffer);
			system::error_code error;
			socket.send_to(buffer, remoteEndpoint, 0, error);
		}
	}
}

void Server::checkInactive() {
	uint64 approxTimeNow = getMilliTimeSince(approxStartTimePoint);
	for (uint8 id = 0; id < MAX_CLIENTS; ++id) {
		if (!clients[id].connected)
			continue;

		if (approxTimeNow - clients[id].timeOfLastPacket > timeout) {
			LOG(INFO, "Player " << (int) id << " timed out");

			char *cursor = outBuffer;
			writeHeader(&cursor, CONNECTION_TIMEOUT);
			auto buffer = asio::buffer(outBuffer, cursor - outBuffer);
			system::error_code error;
			socket.send_to(buffer, clients[id].endpoint, 0, error);

			disconnect(id);
		}
	}
}

void Server::handleConnectionRequest() {
	LOG(INFO, "New connection from IP "
			<< remoteEndpoint.address().to_string()
			<< " and port " << remoteEndpoint.port()
	);

	// find a free spot for the new player
	int newPlayer = -1;
	int duplicate = -1;
	for (int i = 0; i < (int) MAX_CLIENTS; i++) {
		bool connected = clients[i].connected;
		if (newPlayer == -1 && !connected) {
			newPlayer = i;
		}
		if (connected && clients[i].endpoint == remoteEndpoint) {
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
	char *outDataCursor = outBuffer;
	if (!failure) {
		uint8 id = (uint) newPlayer;
		uint32 token = 0; //TODO

		clients[id].connected = true;
		clients[id].endpoint = remoteEndpoint;
		clients[id].token = token;
		clients[id].timeOfLastPacket = getMilliTimeSince(approxStartTimePoint);

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
	auto buffer = asio::buffer(outBuffer, outDataCursor - outBuffer);
	system::error_code error;
	socket.send_to(buffer, remoteEndpoint, 0, error);
	if (error) {
		LOG(ERROR, "Could not send message");
	}
}

void Server::handleClientMessage(uint8 type, uint8 id, const char *data, const char *dataEnd) {
	LOG(TRACE, "Message " << (int) type << " for Player " << (int) id << " accepted");
	switch (type) {
	case ECHO_REQUEST: {
		char *outDataCursor = outBuffer;
		writeHeader(&outDataCursor, ECHO_RESPONSE);
		writeBytes(&outDataCursor, data, dataEnd - data);
		auto buffer = asio::buffer(outBuffer, outDataCursor - outBuffer);
		system::error_code error;
		socket.send_to(buffer, clients[id].endpoint, 0, error);
		if (error) {
			LOG(ERROR, "Could not send message");
		} else {
			LOG(DEBUG, "Echo message by player " << (int) id
					<< " with length " << dataEnd - data);
		}
		break;
	}
	default:
		break;
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
